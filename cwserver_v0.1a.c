#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <sys/time.h>
#include <stdbool.h>
#include <getopt.h> // Added for getopt.h

#define LISTENQ  1024
#define MAXLINE 8192 // Increased MAXLINE to 8192 to match previous code
#define RIO_BUFSIZE 8192

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
    int rio_fd;
    int rio_cnt;
    char *rio_bufptr;
    char rio_buf[RIO_BUFSIZE];
} rio_t;

typedef struct sockaddr SA;

typedef struct http_request {
    char filename[PATH_MAX];
    off_t offset;
    size_t end;
} http_request;

typedef struct {
    const char *extension;
    const char *mime_type;
} mime_map;

// Global variables for pseudo-FTP
char pftp_path_prefix[MAXLINE] = "";
char ftp_password[MAXLINE] = "";

void client_error(int fd, int status, const char *msg, const char *longmsg);
void handle_directory_request(int out_fd, const char *dirname, const char *icon_style);
static const char* get_mime_type(const char *filename);
static const char* get_file_icon(const char *filename, const char *icon_style);
int open_listenfd(const char *port);
void url_decode(const char *src, char *dest, int max);
void parse_request(int fd, http_request *req);
void log_message(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_access(int status, struct sockaddr_in *c_addr, http_request *req);
ssize_t writen(int fd, const void *usrbuf, size_t n);
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
void format_size(char *buf, off_t size);
void serve_static(int out_fd, const char *filename, http_request *req, size_t total_size, bool is_ftp_mode);
void process(int fd, struct sockaddr_in *clientaddr, const char *icon_style);
void *connection_handler(void *socket_desc);
void daemonize_process();
void usage(char *program_name);
void print_version();

static const mime_map meme_types[] = {
    {".css", "text/css"},
    {".gif", "image/gif"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".ico", "image/x-icon"},
    {".js", "application/javascript"},
    {".pdf", "application/pdf"},
    {".djvu", "image/vnd.djvu"},
    {".mp4", "video/mp4"},
    {".webm", "video/webm"},
    {".ogg", "video/ogg"},
    {".mp3", "audio/mpeg"},
    {".wav", "audio/wav"},
    {".flac", "audio/flac"},
    {".aac", "audio/aac"},
    {".flv", "video/x-flv"},
    {".m2v", "video/mpeg"},
    {".ogx", "application/ogg"},
    {".avi", "video/x-msvideo"},
    {".mkv", "video/x-matroska"},
    {".mov", "video/quicktime"},
    {".png", "image/png"},
    {".svg", "image/svg+xml"},
    {".xml", "text/xml"},
    {".json", "application/json"},
    {NULL, NULL},
};

static const char *default_mime_type = "text/plain";
const char *default_icon_style = "text";
char icon_style_str[MAXLINE];

static const char *text_icons[] = {
    "[D]", "[TXT]", "[IMG]", "[VID]", "[AUD]", "[DOC]", "[CODE]", "[ZIP]", "[EXE]", "[FILE]"
};

static const char *emoji_icons[] = {
    "[📂]", "[📝]", "[🖼️]", "[🎥]", "[🎵]", "[📄]", "[💻]", "[📦]", "[⚙️]", "[📃]"
};

void log_message(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

void log_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

void log_access(int status, struct sockaddr_in *c_addr, http_request *req) {
    log_message("%s:%d %d - %s\n", inet_ntoa(c_addr->sin_addr),
                ntohs(c_addr->sin_port), status, req->filename);
}

void rio_readinitb(rio_t *rp, int fd) {
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

ssize_t writen(int fd, const void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    const char *bufp = usrbuf;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}

static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n) {
    int cnt;
    while (rp->rio_cnt <= 0) {
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0) {
            if (errno == EINTR)
                return -1;
        } else if (rp->rio_cnt == 0)
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf;
    }

    cnt = n;
    if (rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {
        if ((rc = rio_read(rp, &c, 1)) == 1) {
            *bufp++ = c;
            if (c == '\n')
                break;
        } else if (rc == 0) {
            if (n == 1)
                return 0;
            else
                break;
        } else
            return -1;
    }
    *bufp = 0;
    return n;
}

void format_size(char *buf, off_t size) {
    if (size < 1024) {
        snprintf(buf, 16, "%lu", size);
    } else if (size < 1024 * 1024) {
        snprintf(buf, 16, "%.1fK", (double)size / 1024);
    } else if (size < 1024 * 1024 * 1024) {
        snprintf(buf, 16, "%.1fM", (double)size / 1024 / 1024);
    } else {
        snprintf(buf, 16, "%.1fG", (double)size / 1024 / 1024 / 1024);
    }
}

void handle_directory_request(int out_fd, const char *dirname, const char *icon_style) {
    char buf[MAXLINE], m_time[32], size[16];
    struct stat statbuf;

    printf("handle_directory_request: ENTERED, dirname='%s', icon_style='%s'\n", dirname, icon_style);
    printf("handle_directory_request: dirname = '%s', icon_style = '%s'\n", dirname, icon_style);

    snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n");
    printf("handle_directory_request: writing HTTP header\n");
    writen(out_fd, buf, strlen(buf));
    printf("handle_directory_request: HTTP header written\n");

    snprintf(buf, sizeof(buf),
             "<html><head><title>Directory listing for %s</title><style>"
             "body{font-family: monospace; font-size: 13px;}"
             "td {padding: 1.5px 6px;}"
             "</style></head><body><h1>Directory listing for %s</h1><hr><table>\n",
             dirname, dirname);
    printf("handle_directory_request: writing HTML header\n");
    writen(out_fd, buf, strlen(buf));
    printf("handle_directory_request: HTML header written\n");

    printf("handle_directory_request: opening directory '%s'\n", dirname);
    DIR *d = opendir(dirname);
    if (!d) {
        int errsv = errno;
        log_error("opendir(%s) failed: %s\n", dirname, strerror(errsv));
        client_error(out_fd, 500, "Internal Server Error", "Failed to open directory");
        return;
    }
    printf("handle_directory_request: directory '%s' opened successfully\n", dirname);

    struct dirent *dp;
    printf("handle_directory_request: starting readdir loop\n");
    while ((dp = readdir(d)) != NULL) {
        printf("handle_directory_request: readdir returned entry '%s'\n", dp->d_name);
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
            printf("handle_directory_request: skipping '.' or '..'\n");
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dirname, dp->d_name);
        printf("handle_directory_request: full_path = '%s'\n", full_path);

        printf("handle_directory_request: calling stat('%s')\n", full_path);
        if (stat(full_path, &statbuf) == -1) {
            int errsv = errno;
            log_error("stat(%s) failed: %s\n", full_path, strerror(errsv));
            continue;
        }
        printf("handle_directory_request: stat('%s') successful\n", full_path);

        printf("handle_directory_request: statbuf.st_mtime = %ld\n", (long)statbuf.st_mtime); // **ДОБАВЛЕНО DEBUG LOG**
        strftime(m_time, sizeof(m_time), "%Y-%m-%d %H:%M", localtime(&statbuf.st_mtime));
        format_size(size, statbuf.st_size);

        char *display_name = dp->d_name;
        printf("handle_directory_request: display_name = '%s'\n", display_name);

        char *dir_indicator = (S_ISDIR(statbuf.st_mode)) ? "/" : "";
        printf("handle_directory_request: dir_indicator = '%s'\n", dir_indicator);

        printf("handle_directory_request: icon_style перед выбором иконки = '%s'\n", icon_style); // **Debug: icon_style перед выбором иконки**
        const char *icon = S_ISDIR(statbuf.st_mode) ?
            (strcmp(icon_style, "emoji") == 0 ? emoji_icons[0] : text_icons[0]) :
            get_file_icon(dp->d_name, icon_style);
        printf("handle_directory_request: icon = '%s'\n", icon);

        char escaped_name[MAXLINE];
        int j = 0;
        for (int i = 0; display_name[i] != '\0' && j < MAXLINE - 1; i++) {
          escaped_name[j++] = display_name[i];
        }
        escaped_name[j] = '\0';
        printf("handle_directory_request: escaped_name = '%s'\n", escaped_name);


        printf("handle_directory_request: snprintf for table row, icon_style = '%s'\n", icon_style);
        if (strcmp(icon_style, "none") == 0) {
            snprintf(buf, sizeof(buf),
                     "<tr><td><a href=\"%s%s\">%s%s</a></td><td>%s</td><td>%s</td></tr>\n",
                     escaped_name, dir_indicator, escaped_name, dir_indicator, m_time, size);
        } else {
            snprintf(buf, sizeof(buf),
                     "<tr><td>%s</td><td><a href=\"%s%s\">%s%s</a></td><td>%s</td><td>%s</td></tr>\n",
                     icon, escaped_name, dir_indicator, escaped_name, dir_indicator, m_time, size);
        }
        printf("handle_directory_request: table row snprintf complete\n");
        printf("handle_directory_request: writing table row\n");
        writen(out_fd, buf, strlen(buf));
        printf("handle_directory_request: table row written\n");
    }
    printf("handle_directory_request: readdir loop finished\n");

    snprintf(buf, sizeof(buf), "</table><hr></body></html>");
    printf("handle_directory_request: writing HTML footer\n");
    writen(out_fd, buf, strlen(buf));
    printf("handle_directory_request: HTML footer written\n");

    closedir(d);
    printf("handle_directory_request: directory '%s' closed\n", dirname);
    printf("handle_directory_request: function finished successfully\n");
}



static const char* get_mime_type(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (dot) {
        for (const mime_map *map = meme_types; map->extension != NULL; map++) {
            if (strcmp(map->extension, dot) == 0) {
                return map->mime_type;
            }
        }
    }
    return default_mime_type;
}

static const char* get_file_icon(const char *filename, const char *icon_style) {
    printf("get_file_icon: filename = '%s', icon_style = '%s'\n", filename, icon_style); // Debug: filename and icon_style

    const char *dot = strrchr(filename, '.');
    if (dot == NULL) {
        return (strcmp(icon_style, "emoji") == 0) ? emoji_icons[9] : text_icons[9];
    }

    const char *ext = dot + 1;

    if (strcmp(icon_style, "emoji") == 0) {
        if (strcasecmp(ext, "txt") == 0 || strcasecmp(ext, "log") == 0 || strcasecmp(ext, "conf") == 0 || strcasecmp(ext, "ini") == 0 || strcasecmp(ext, "csv") == 0) return emoji_icons[1];
        else if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0 || strcasecmp(ext, "png") == 0 || strcasecmp(ext, "gif") == 0 || strcasecmp(ext, "ico") == 0 || strcasecmp(ext, "svg") == 0 || strcasecmp(ext, "webp") == 0) return emoji_icons[2];
        else if (strcasecmp(ext, "mp4") == 0 || strcasecmp(ext, "webm") == 0 || strcasecmp(ext, "ogg") == 0 || strcasecmp(ext, "avi") == 0 || strcasecmp(ext, "flv") == 0 || strcasecmp(ext, "m2v") == 0 || strcasecmp(ext, "ogx") == 0 || strcasecmp(ext, "mkv") == 0 || strcasecmp(ext, "mov") == 0) return emoji_icons[3];
        else if (strcasecmp(ext, "mp3") == 0 || strcasecmp(ext, "wav") == 0 || strcasecmp(ext, "flac") == 0 || strcasecmp(ext, "aac") == 0) return emoji_icons[4];
        else if (strcasecmp(ext, "pdf") == 0 || strcasecmp(ext, "djvu") == 0) return emoji_icons[5];
        else if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0 || strcasecmp(ext, "css") == 0 || strcasecmp(ext, "js") == 0 || strcasecmp(ext, "xml") == 0 || strcasecmp(ext, "json") == 0 || strcasecmp(ext, "c") == 0 || strcasecmp(ext, "cpp") == 0 || strcasecmp(ext, "h") == 0 || strcasecmp(ext, "py") == 0 || strcasecmp(ext, "java") == 0 || strcasecmp(ext, "php") == 0 || strcasecmp(ext, "sh") == 0) return emoji_icons[6];
        else if (strcasecmp(ext, "zip") == 0 || strcasecmp(ext, "tar") == 0 || strcasecmp(ext, "gz") == 0 || strcasecmp(ext, "bz2") == 0 || strcasecmp(ext, "rar") == 0 || strcasecmp(ext, "7z") == 0) return emoji_icons[7];
        else if (strcasecmp(ext, "exe") == 0) return emoji_icons[8];
        else return emoji_icons[9];
    } else {
        if (strcasecmp(ext, "txt") == 0 || strcasecmp(ext, "log") == 0 || strcasecmp(ext, "conf") == 0 || strcasecmp(ext, "ini") == 0 || strcasecmp(ext, "csv") == 0) return text_icons[1];
        else if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0 || strcasecmp(ext, "png") == 0 || strcasecmp(ext, "gif") == 0 || strcasecmp(ext, "ico") == 0 || strcasecmp(ext, "svg") == 0 || strcasecmp(ext, "webp") == 0) return text_icons[2];
        else if (strcasecmp(ext, "mp4") == 0 || strcasecmp(ext, "webm") == 0 || strcasecmp(ext, "ogg") == 0 || strcasecmp(ext, "avi") == 0 || strcasecmp(ext, "flv") == 0 || strcasecmp(ext, "m2v") == 0 || strcasecmp(ext, "ogx") == 0 || strcasecmp(ext, "mkv") == 0 || strcasecmp(ext, "mov") == 0) return text_icons[3];
        else if (strcasecmp(ext, "mp3") == 0 || strcasecmp(ext, "wav") == 0 || strcasecmp(ext, "flac") == 0 || strcasecmp(ext, "aac") == 0) return text_icons[4];
        else if (strcasecmp(ext, "pdf") == 0 || strcasecmp(ext, "djvu") == 0) return text_icons[5];
        else if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0 || strcasecmp(ext, "css") == 0 || strcasecmp(ext, "js") == 0 || strcasecmp(ext, "xml") == 0 || strcasecmp(ext, "json") == 0 || strcasecmp(ext, "c") == 0 || strcasecmp(ext, "cpp") == 0 || strcasecmp(ext, "h") == 0 || strcasecmp(ext, "py") == 0 || strcasecmp(ext, "java") == 0 || strcasecmp(ext, "php") == 0 || strcasecmp(ext, "sh") == 0) return text_icons[6];
        else if (strcasecmp(ext, "zip") == 0 || strcasecmp(ext, "tar") == 0 || strcasecmp(ext, "gz") == 0 || strcasecmp(ext, "bz2") == 0 || strcasecmp(ext, "rar") == 0 || strcasecmp(ext, "7z") == 0) return emoji_icons[7];
        else if (strcasecmp(ext, "exe") == 0) return text_icons[8];
        else return text_icons[9];
    }
}

int open_listenfd(const char *port) {
    struct addrinfo hints, *listp, *p;
    int listenfd, optval = 1, rc;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_protocol = 0;

    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
        log_error("getaddrinfo error: %s\n", gai_strerror(rc));
        return -2;
    }

    for (p = listp; p; p = p->ai_next) {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
        setsockopt(listenfd, IPPROTO_TCP, TCP_CORK, (const void*)&optval, sizeof(int));

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        close(listenfd);
    }

    freeaddrinfo(listp);
    if (!p)
        return -1;

    if (listen(listenfd, LISTENQ) < 0) {
        close(listenfd);
        return -1;
    }

    return listenfd;
}

void url_decode(const char *src, char *dest, int max) {
    const char *p = src;
    char *q = dest;
    int i;
    char code[3] = {0};

    while (*p != '\0' && q < dest + max - 1) {
        if (*p == '%') {
            if (strlen(p) < 2) break;
            p++;
            code[0] = *p++;
            code[1] = *p++;

            if (!isxdigit(code[0]) || !isxdigit(code[1])) {
                *q++ = '?';
                continue;
            }

            char *endptr;
            long value = strtol(code, &endptr, 16);

            if (*endptr != '\0') {
                *q++ = '?';
                continue;
            }

            *q++ = (char)value;
        } else {
            *q++ = *p++;
        }
    }
    *q = '\0';
}

void parse_request(int fd, http_request *req) {
    rio_t rio;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE];
    req->offset = 0;
    req->end = 0;

    rio_readinitb(&rio, fd);

    if (rio_readlineb(&rio, buf, MAXLINE) <= 0) {
        log_error("Failed to read request line\n");
        return;
    }

    if (sscanf(buf, "%s %s", method, uri) != 2) {
        log_error("Failed to parse request line: %s\n", buf);
        return;
    }

    printf("parse_request: Original URI = '%s'\n", uri); // **ОТЛАДОЧНАЯ ПЕЧАТЬ (перед url_decode)**

    char* filename = uri;
    if (uri[0] == '/') {
        filename = uri + 1;
        int length = strlen(filename);
        if (length == 0) {
            filename = ".";
        } else {
            for (int i = 0; i < length; ++i) {
                if (filename[i] == '?') {
                    filename[i] = '\0';
                    break;
                }
            }
        }
    }

    url_decode(filename, req->filename, sizeof(req->filename));
    printf("parse_request: Decoded filename = '%s'\n", req->filename); // **ОТЛАДОЧНАЯ ПЕЧАТЬ (после url_decode)**
}


void client_error(int fd, int status, const char *msg, const char *longmsg) {
    char buf[MAXLINE];
    snprintf(buf, sizeof(buf), "HTTP/1.1 %d %s\r\n", status, msg);
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "Content-length: %lu\r\n", strlen(longmsg));
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "Content-type: text/plain\r\n\r\n");
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", longmsg);
    writen(fd, buf, strlen(buf));
}

bool is_video_mime_type(const char *mime_type) {
    return strncmp(mime_type, "video/", 6) == 0;
}

bool is_audio_mime_type(const char *mime_type) {
    return strncmp(mime_type, "audio/", 6) == 0;
}

// Обслуговування статичного файлу
void serve_static(int out_fd, const char *filename, http_request *req, size_t total_size, bool is_ftp_mode) { // is_ftp_mode is present for consistency
    char buf[MAXLINE];
    int in_fd;
    const char *mime_type;

    char resolved_path[PATH_MAX];
    if (realpath(filename, resolved_path) == NULL) { // Розв'язання відносного шляху в абсолютний
        client_error(out_fd, 403, "Forbidden", "Invalid path"); // Помилка: недійсний шлях
        return;
    }

    if (strncmp(resolved_path, "/tmp", strlen("/tmp")) != 0) { // Перевірка, чи шлях не виходить за межі /tmp (безпека)
        client_error(out_fd, 403, "Forbidden", "Access denied"); // Помилка доступу: шлях за межами /tmp
        return;
    }

    in_fd = open(filename, O_RDONLY, 0); // Відкриття файлу для читання
    if (in_fd < 0) {
        client_error(out_fd, 404, "Not found", "File not found"); // Помилка: файл не знайдено
        return;
    }

    mime_type = get_mime_type(filename); // Get mime type here, as it's needed in both modes - Отримання MIME-типу тут, оскільки він потрібен в обох режимах

    // In this version, is_ftp_mode is not actually used in serve_static
    if (is_ftp_mode) { // This block is present but doesn't change behavior in this version
        if (is_video_mime_type(mime_type)) {
            snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
            writen(out_fd, buf, strlen(buf));

            char video_html[MAXLINE * 4];
            snprintf(video_html, sizeof(video_html),
                     "<!DOCTYPE html><html><head><title>Video Player</title><style>"
                     "body { font-family: sans-serif; text-align: center; }"
                     "h1 { font-size: 1.2em; color: #666; }"
                     "#video-container { display: inline-block; }"
                     "</style></head><body>"
                     "<div id=\"video-container\">"
                     "<h1>Playing: %s</h1>"
                     "<video width=\"640\" height=\"480\" controls>"
                     "<source src=\"/%s\" type=\"%s\">"
                     "Your browser does not support the video tag."
                     "</video>"
                     "</div></body></html>",
                     filename, filename, mime_type);

            writen(out_fd, video_html, strlen(video_html));
            close(in_fd);
            return;

        } else {
            goto serve_file_static;
        }


    } else {
        goto serve_file_static;
    }


serve_file_static:
     if (req->offset > 0) {
            snprintf(buf, sizeof(buf), "HTTP/1.1 206 Partial Content\r\n");
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "Content-Range: bytes %lu-%lu/%lu\r\n",
                     req->offset, req->end - 1, total_size);
        } else {
            snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\nAccept-Ranges: bytes\r\n");
        }
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "Cache-Control: no-cache\r\n");
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "Content-Length: %lu\r\n",
                 req->end - req->offset);
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "Content-Type: %s\r\n\r\n",
                 mime_type);

        writen(out_fd, buf, strlen(buf));

        off_t offset = req->offset;
        ssize_t sf_result;
        while (offset < req->end) {
            size_t bytes_to_send = req->end - offset;
            sf_result = sendfile(out_fd, in_fd, &offset, bytes_to_send);

            if (sf_result <= 0) {
                if (errno == EINTR) continue;
                if (errno == EPIPE) {
                    log_message("Client disconnected prematurely (Broken pipe)\n");
                    break;
                }
                log_error("sendfile error: %s\n", strerror(errno));
                break;
            }
        }


    close(in_fd);
}

void process(int fd, struct sockaddr_in *clientaddr, const char *icon_style) {
    printf("process: icon_style = %s\n", icon_style);
    printf("accept request, fd is %d, pid is %d\n", fd, getpid());
    bool is_ftp_mode = false; // Initialize is_ftp_mode here

    http_request req; // Declare req here
    parse_request(fd, &req);

    if (strlen(pftp_path_prefix) > 0) {
        printf("process: pftp_path_prefix = '%s', length = %lu\n", pftp_path_prefix, strlen(pftp_path_prefix)); // **ОТЛАДОЧНАЯ ПЕЧАТЬ**

        if (strncmp(req.filename, pftp_path_prefix + 1, strlen(pftp_path_prefix) - 1) == 0) { // **УПРОЩЕННАЯ ПРОВЕРКА ПРЕФИКСА!**
            is_ftp_mode = true;
            printf("Debug: FTP mode activated for filename: %s, prefix: %s\n", req.filename, pftp_path_prefix); // Debug log for FTP mode

            // Remove prefix from req.filename
            memmove(req.filename, req.filename + strlen(pftp_path_prefix) - 1, strlen(req.filename) - (strlen(pftp_path_prefix) - 1) + 1); // +1 for null terminator
            printf("Debug: After prefix removal for nested dir, filename is now: %s\n", req.filename); // **ОТЛАДОЧНАЯ ПЕЧАТЬ**
            if (strlen(req.filename) == 0) { // **Check for empty req.filename after prefix removal**
                strcpy(req.filename, "."); // **Set req.filename to "." if it's empty**
                printf("Debug: filename set to '.' (root of FTP)\n"); // Debug log for setting filename to "."
            }

            printf("Debug: Prefix removed, filename is now: %s\n", req.filename); // Debug log after prefix removal


        } else {
            printf("Debug: FTP prefix not found in filename: %s, prefix: %s\n", req.filename, pftp_path_prefix); // Debug log for no FTP mode
        }
    } else {
        printf("Debug: FTP password not set, pseudo-FTP mode disabled\n"); // Debug log for FTP disabled
    }

    struct stat sbuf;
    int status = 200;
    int ffd;

    printf("filename from process = %s\n", req.filename);

    // if (strncmp(req.filename, "ftp/", 4) == 0) { // Removed ftp block in previous step - still commented out
    //     is_ftp_mode = true;
    //     memmove(req.filename, req.filename + 4, strlen(req.filename) - 3);
    // }

    if (is_ftp_mode && strlen(req.filename) == 0) { // This part remains as in the "previous" version
        strcpy(req.filename, ".");
    }

    ffd = open(req.filename, O_RDONLY, 0);
    if (ffd <= 0) {
        status = 404;
        char *msg = "File not found";
        client_error(fd, status, "Not found", msg);
    } else {
        fstat(ffd, &sbuf);

        if (S_ISDIR(sbuf.st_mode)) {
            if (is_ftp_mode) { // This block is present, behavior will be modified in later steps
                close(ffd);
                status = 200;
                handle_directory_request(fd, req.filename, icon_style);
                log_access(status, clientaddr, &req);
                return;
            } else { // Standard HTTP directory handling path - **MODIFIED for Variant 2**
                char index_path[PATH_MAX];
                if (req.filename[strlen(req.filename) - 1] == '/') {
                    snprintf(index_path, sizeof(index_path), "%sindex.html", req.filename);
                } else {
                    snprintf(index_path, sizeof(index_path), "%s/index.html", req.filename);
                }

                close(ffd);
                ffd = open(index_path, O_RDONLY, 0);
                if (ffd < 0) {
                    status = 404; // **RETURN 404 Not Found if index.html is not found**
                    client_error(fd, status, "Not found", "File not found"); // **RETURN 404 Not Found**
                    log_access(status, clientaddr, &req);
                    return;
                }
                fstat(ffd, &sbuf);

                if (req.end == 0) {
                    req.end = sbuf.st_size;
                }
                if (req.offset > 0) {
                    status = 206;
                }
                serve_static(fd, index_path, &req, sbuf.st_size, is_ftp_mode); // is_ftp_mode is passed
            }
        } else if (S_ISREG(sbuf.st_mode)) { // Standard HTTP file serving path
            if (req.end == 0) {
                req.end = sbuf.st_size;
            }
            if (req.offset > 0) {
                status = 206;
            }
            serve_static(fd, req.filename, &req, sbuf.st_size, is_ftp_mode); // is_ftp_mode is passed
        } else {
            status = 400;
            char *msg = "Unknown Error";
            client_error(fd, status, "Error", msg);
        }
        close(ffd);
    }

    log_access(status, clientaddr, &req);
}


void *connection_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    const char *icon_style = icon_style_str;

    printf("connection_handler: icon_style = %s\n", icon_style);

    if (getpeername(sock, (SA *)&clientaddr, &clientlen) == -1) {
        perror("getpeername");
        log_error("Failed to get client address\n");
        return NULL;
    }

    printf("Handling connection in thread, fd is %d, icon style: %s\n", sock, icon_style);
    process(sock, &clientaddr, icon_style);
    close(sock);
    free(socket_desc);
    return NULL;
}

void daemonize_process() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) {
            close(fd);
        }
    }

    umask(0);
}

void usage(char *program_name) {
    fprintf(stderr, "Usage: %s [-p port] [-w web_root] [-d] [-h] [-v] [-i icon_style] [-f ftp_password]\n", program_name);
    fprintf(stderr, "  -p port      Specify the port to listen on (default: 8080)\n");
    fprintf(stderr, "  -w web_root  Specify the web root directory (default: .)\n");
    fprintf(stderr, "  -d           Run in daemon mode\n");
    fprintf(stderr, "  -h           Show this help message\n");
    fprintf(stderr, "  -v           Show version information\n");
    fprintf(stderr, "  -i icon_style Specify icon style for directory listing (default: text)\n");
    fprintf(stderr, "                 Possible values: text, emoji, none\n");
    fprintf(stderr, "  -f ftp_password Enable pseudo-FTP mode with password-based path prefix\n"); // Added -f option description
    exit(EXIT_FAILURE);
}

void print_version() {
    printf("cWServer (Version 0.1a) - Lightweight, Multi-Threaded Web Server\n");
    printf("Copyright (C) 2025 Ivan Svarkovsky <https://github.com/Svarkovsky>\n");
    printf("Licensed under GPLv2+ <http://www.gnu.org/licenses/gpl.html>\n");
    printf("\n");
    printf("Designed for serving static content with concurrent connection handling via POSIX threads.\n");
    printf("Primarily intended for educational use.\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
    int listenfd, connfd, *new_sock;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char port[MAXLINE];
    char web_root[MAXLINE];
    pthread_t thread_id;
    int daemonize = 0;
    int option_char; // For getopt

    snprintf(port, MAXLINE, "8080");
    snprintf(web_root, MAXLINE, ".");
    snprintf(icon_style_str, MAXLINE, default_icon_style);

    while ((option_char = getopt(argc, argv, "p:w:dhvi:f:")) != -1) {
        switch (option_char) {
        case 'p':
            strncpy(port, optarg, MAXLINE - 1);
            port[MAXLINE - 1] = '\0';
            break;
        case 'w':
            strncpy(web_root, optarg, MAXLINE - 1);
            web_root[MAXLINE - 1] = '\0';
            break;
        case 'd':
            daemonize = 1;
            break;
        case 'h':
            usage(argv[0]);
            break;
        case 'v':
            print_version();
            break;
        case 'i':
            strncpy(icon_style_str, optarg, MAXLINE - 1);
            icon_style_str[MAXLINE - 1] = '\0';
            break;
        case 'f': // Handle -f option for ftp_password
            strncpy(ftp_password, optarg, MAXLINE - 1);
            ftp_password[MAXLINE - 1] = '\0';
            snprintf(pftp_path_prefix, MAXLINE, "/%s/", ftp_password); // Construct pftp_path_prefix
            printf("Debug: Pseudo-FTP password set, path prefix: %s\n", pftp_path_prefix); // Debug print
            break;
        default:
            usage(argv[0]);
        }
    }

    printf("main: icon_style_str = %s\n", icon_style_str);

    if (chdir(web_root) != 0) {
        perror(web_root);
        exit(EXIT_FAILURE);
    }

    if (daemonize) {
        daemonize_process();
    }

    listenfd = open_listenfd(port);
    if (listenfd > 0) {
        printf("listen on port %s, fd is %d\n", port, listenfd);
    } else {
        log_error("ERROR opening listen socket\n");
        exit(EXIT_FAILURE);
    }

    signal(SIGPIPE, SIG_IGN);

    while ((connfd = accept(listenfd, (SA *)&clientaddr, &clientlen))) {
        if (connfd < 0) {
            perror("accept");
            continue;
        }

        new_sock = malloc(sizeof(int));
        *new_sock = connfd;

        if (pthread_create(&thread_id, NULL, connection_handler, (void*)new_sock) < 0) {
            perror("could not create thread");
            free(new_sock);
            close(connfd);
            continue;
        }

        pthread_detach(thread_id);
    }

    close(listenfd);

    return 0;
}


