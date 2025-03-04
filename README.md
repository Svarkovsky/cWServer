# cWServer - Lightweight, Multithreaded Web Server

[Перейти до української версії](#cWServer---Легкий-багатопотоковий-веб-сервер)  
[前往中文版本](#cWServer---轻量级多线程Web服务器)

<br> <div align="center"> <img src="unCs9b5zHjUiwcKv-generated_image.jpg" alt="cWServer Logo" style="width: 300px; height: auto;"> </div> <br>

**cWServer** is a **lightweight, multithreaded web server written in C using POSIX standard libraries**. It is designed to efficiently serve static content and features a multithreaded architecture to handle concurrent connections. The server is primarily intended for educational purposes, demonstrating the basic principles of web servers, but can also be used for simple static website hosting tasks. The simplicity of the code and the use of standard libraries make it easy to understand, modify, and port to various platforms.

## Key Features

- **Serving Static Files:** cWServer efficiently serves static files such as HTML, CSS, JavaScript, images, video, and audio.
- **Multithreaded Architecture:** The server uses POSIX threads (`pthreads`) to handle each incoming connection in a separate thread, ensuring good performance under high concurrent loads.
- **Partial Content Support (Content-Range):** The server supports `Content-Range` HTTP headers, allowing clients to download files in parts, which is useful for large files or interrupted connections.
- **Caching Disabled (Cache-Control: no-cache):** By default, the server disables client-side caching to ensure the latest content is served.
- **Directory Listing:** If a directory is requested instead of a specific file, the server generates an HTML page listing the files in that directory.
- **Directory Listing Styling:** Ability to choose icon styles for directory listings:
  - **text:** Text icons like `[D]`, `[TXT]`, `[IMG]`, etc.
  - **emoji:** Emoji icons like `[📂]`, `[📝]`, `[🖼️]`, etc.
  - **none:** No icons.
- **Protected Directory View Mode (formerly Pseudo-FTP):** A password-protected directory view mode. It restricts access to certain directories using a password as a path prefix.
- **Daemon Mode:** Ability to run the server in the background as a daemon.
- **Detailed Logging:** The server logs access and errors to standard error output (`stderr`).
- **URL-encoded Request Handling:** The server correctly handles URL-encoded characters in requests.
- **index.html Handling:** When a directory is requested, the server first looks for an `index.html` file in that directory. If found, it serves the file. If not, it returns a directory listing (unless Protected Directory View is enabled).

## Architecture

The current build of the server is optimized for the **MIPS architecture (74kc, mips16, mdsp, EB)**. The Makefile contains compiler and linker settings specific to this architecture, including:

- `-march=74kc -mips16 -mdsp`: GCC options for optimizing for the MIPS 74kc architecture with MIPS16 and DSP instructions.
- `-EB`: Specifies big-endian byte order (if required).
- Specific linking and strip options to reduce the executable file size.

**Building for Other Architectures:**

To build for other architectures, you may need to modify the `CFLAGS` and `LDFLAGS` variables in the `Makefile`. Specifically, you will need to adjust:

- `-march=...`: Specify the target architecture for your processor.
- `-mips16 -mdsp -EB`: Remove or modify these options if they are not supported by your architecture or are not needed.
- `--sysroot=... -I...`: Check and update the paths to system include and lib directories if they differ in your system.

**Important:** Users are free to modify and distribute the server code under the terms of the GPLv2 or later license.

## Getting Started

### Compilation

To compile the server, you will need a C compiler (e.g., GCC) and `make`.

1. Ensure you have the necessary development tools installed (GCC, make, and other required libraries).
2. Save the `cwserver_v0.1a.c` and `Makefile` files in the same directory.
3. Open a terminal in this directory.
4. Run the `make` command:

    ```bash
    make
    ```

    This will create an executable file named `cwserver`.

### Running

To run the server, use the following command:

```bash
./cwserver [options]
```

### Options

- **`-p port`**  
  Specifies the port to listen on. Use this option to set the port on which the web server will wait for incoming connections.  
  **Default:** `8080`  
  ```bash
  ./cwserver -p 3000
  ```
- **`-w web_root`**  
  Specifies the web server's root directory. Sets the directory on the file system from which the server will serve files to clients.  
  **Default:** Current directory (`.`)  
  ```bash
  ./cwserver -w /var/www/html
  ```
- **`-d`**  
  Runs the server in daemon mode (background mode). When this option is used, the server runs in the background, detached from the terminal. This is useful for long-term server operation.  
  ```bash
  ./cwserver -d
  ```
- **`-h`**  
  Displays a help message with a description of the options. Using this option will print a help message describing all available command-line options and terminate the server.  
  ```bash
  ./cwserver -h
  ```
- **`-v`**  
  Displays the server version information. Using the `-v` option will print the `cwServer` version information and terminate the server.  
  ```bash
  ./cwserver -v
  ```
- **`-i icon_style`**  
  Specifies the icon style for directory listings. Allows you to choose the style of icons displayed in the HTML directory listing.  
  **Possible values:** `text`, `emoji`, `none`  
  **Default:** `text`  
  ```bash
  ./cwserver -i emoji
  ```
- **`-f ftp_password`**  
  Enables Pseudo-FTP mode with the password `ftp_password`. Activates restricted directory access using the password as a path prefix.  
  ```bash
  ./cwserver -f mypassword
  ```

## Usage Examples

1. Running the server on port `3000` with the root directory `/var/www/html`:  
   ```bash
   ./cwserver -p 3000 -w /var/www/html
   ```

2. Running the server in daemon mode with `emoji` icon style:  
   ```bash
   ./cwserver -d -i emoji
   ```

3. Displaying help:  
   ```bash
   ./cwserver -h
   ```

4. Checking the server version:  
   ```bash
   ./cwserver -v
   ```

## License

cWServer is distributed under the **GPLv2 or later** license.

This project is licensed under the terms of the **GNU General Public License version 2** or any later version (GPLv2+).

You have the full freedom to:

- Use the software for any purpose.
- Study how the software works and adapt it to your needs.
- Distribute copies of the software.
- Improve the software and publish your improvements.

The full license text is available at: [http://www.gnu.org/licenses/gpl.html](http://www.gnu.org/licenses/gpl.html).

## Author

**Ivan Svarkovsky** - [https://github.com/Svarkovsky](https://github.com/Svarkovsky)

Made with ❤️ for people.

---

# cWServer - Легкий, багатопотоковий веб-сервер

[Go to English version](#cWServer---Lightweight-Multithreaded-Web-Server)
[前往中文版本](#cWServer---轻量级多线程Web服务器)

<br> <div align="center"> <img src="unCs9b5zHjUiwcKv-generated_image.jpg" alt="cWServer Logo" style="width: 300px; height: auto;"> </div> <br>

**cWServer** - це **легкий та багатопотоковий веб-сервер, написаний на мові програмування C з використанням стандартних бібліотек POSIX**. Він розроблений для ефективного обслуговування статичного контенту та має багатопотокову архітектуру для обробки одночасних з'єднань. Сервер призначений в першу чергу для освітніх цілей, демонстрації основних принципів роботи веб-серверів, але може бути використаний і для простих задач обслуговування статичних веб-сайтів. Простота коду та використання стандартних бібліотек роблять його легким для розуміння, модифікації та перенесення на різні платформи.

## Основні можливості

- **Обслуговування статичних файлів:** cWServer ефективно обслуговує статичні файли, такі як HTML, CSS, JavaScript, зображення, відео та аудіо.
- **Багатопотокова архітектура:** Сервер використовує POSIX потоки (`pthreads`) для обробки кожного вхідного з'єднання в окремому потоці, що забезпечує хорошу продуктивність при великій кількості одночасних запитів.
- **Підтримка часткових запитів (Content-Range):** Сервер підтримує обробку HTTP-заголовків `Content-Range`, дозволяючи клієнтам отримувати файли частинами, що корисно для великих файлів або перерваних з'єднань.
- **Кешування відключено (Cache-Control: no-cache):** За замовчуванням сервер відключає кешування на стороні клієнта, щоб забезпечити віддачу найновішого контенту.
- **Перегляд вмісту директорій:** Якщо в запиті вказана директорія, а не конкретний файл, сервер генерує HTML-сторінку зі списком файлів у цій директорії.
- **Стилізація списку директорій:** Можливість вибору стилю іконок для списку директорій:
  - **text:** Текстові іконки `[D]`, `[TXT]`, `[IMG]` і т.д.
  - **emoji:** Іконки emoji `[📂]`, `[📝]`, `[🖼️]` і т.д.
  - **none:** Без іконок.
- **Режим Protected Directory View (раніше псевдо-FTP):** Режим захищеного перегляду директорій з паролем. Дозволяє обмежити доступ до певних директорій за паролем, використовуючи префікс шляху на основі пароля.
- **Режим демона:** Можливість запуску сервера у фоновому режимі як демон.
- **Детальне логування:** Сервер веде лог доступу та помилок у стандартний вивід помилок (stderr).
- **Обробка URL-encoded запитів:** Сервер коректно обробляє URL-encoded символи у запитах.
- **Обробка index.html:** При запиті директорії сервер спочатку шукає файл `index.html` у цій директорії і, якщо знаходить, обслуговує його. Якщо `index.html` відсутній, сервер повертає список файлів директорії (якщо не увімкнено Protected Directory View).

## Архітектура

Поточна збірка сервера оптимізована для **архітектури MIPS (74kc, mips16, mdsp, EB)**. Makefile містить налаштування компілятора та лінкера, специфічні для цієї архітектури, включаючи:

- `-march=74kc -mips16 -mdsp`: Опції GCC для оптимізації під архітектуру MIPS 74kc з використанням інструкцій MIPS16 та DSP.
- `-EB`: Вказівка на big-endian порядок байтів (якщо потрібно).
- Специфічні опції лінкування та strip для зменшення розміру виконуваного файлу.

**Збірка для інших архітектур:**

Для збірки на інших архітектурах вам може знадобитися змінити значення змінних `CFLAGS` та `LDFLAGS` у `Makefile`. Зокрема, вам потрібно буде скоригувати:

- `-march=...`: Вказати цільову архітектуру для вашого процесора.
- `-mips16 -mdsp -EB`: Видалити або змінити ці опції, якщо вони не підтримуються вашою архітектурою або не потрібні.
- `--sysroot=... -I...`: Перевірити та оновити шляхи до системних include та lib директорій, якщо вони відрізняються у вашій системі.

**Важливо:** Користувачі мають право змінювати та розповсюджувати код сервера відповідно до умов ліцензії GPLv2 або пізнішої.

## Як почати

### Компіляція

Для компіляції сервера вам знадобиться компілятор C (наприклад, GCC) та `make`.

1. Переконайтеся, що у вас встановлено необхідні інструменти розробки (GCC, make, та інші необхідні бібліотеки).
2. Збережіть файл `cwserver_v0.1a.c` та `Makefile` в одній директорії.
3. Відкрийте термінал у цій директорії.
4. Виконайте команду `make`:

    ```bash
    make
    ```

    Це створить виконуваний файл `cwserver`.

### Запуск

Для запуску сервера використовуйте наступну команду:

```bash
./cwserver [опції]
```

### Опції

- **`-p port`**  
  Вказує порт для прослуховування. Використовуйте цю опцію, щоб задати порт, на якому веб-сервер буде очікувати вхідні з'єднання.  
  **За замовчуванням:** `8080`  
  ```bash
  ./cwserver -p 3000
  ```
- **`-w web_root`**  
  Вказує кореневу директорію веб-сервера. Задає директорію на файловій системі, відносно якої сервер буде шукати файли для віддачі клієнтам.  
  **За замовчуванням:** поточна директорія (`.`)  
  ```bash
  ./cwserver -w /var/www/html
  ```
- **`-d`**  
  Запускає сервер у режимі демона (у фоновому режимі). При використанні цієї опції сервер запускається у фоновому режимі, від'єднуючись від терміналу. Це корисно для довготривалої роботи сервера.  
  ```bash
  ./cwserver -d
  ```
- **`-h`**  
  Показує довідкове повідомлення з описом опцій. Використання цієї опції виведе на екран довідку з описом всіх доступних опцій командного рядка та завершить роботу сервера.  
  ```bash
  ./cwserver -h
  ```
- **`-v`**  
  Показує інформацію про версію сервера. Використання опції `-v` виведе на екран інформацію про версію `cwServer` та завершить роботу.  
  ```bash
  ./cwserver -v
  ```
- **`-i icon_style`**  
  Вказує стиль іконок для списку директорій. Дозволяє вибрати стиль відображення іконок у HTML-списку директорій.  
  **Можливі значення:** `text`, `emoji`, `none`  
  **За замовчуванням:** `text`  
  ```bash
  ./cwserver -i emoji
  ```
- **`-f ftp_password`**  
  Вмикає режим Pseudo-FTP з паролем `ftp_password`. Активує режим обмеженого доступу до директорій, використовуючи пароль як префікс шляху.  
  ```bash
  ./cwserver -f mypassword
  ```

## Приклади використання

1. Запуск сервера на порту `3000` з кореневою директорією `/var/www/html`:  
   ```bash
   ./cwserver -p 3000 -w /var/www/html
   ```

2. Запуск сервера у фоновому режимі зі стилем іконок `emoji`:  
   ```bash
   ./cwserver -d -i emoji
   ```

3. Отримання довідки:  
   ```bash
   ./cwserver -h
   ```

4. Перевірка версії сервера:  
   ```bash
   ./cwserver -v
   ```

## Ліцензія

cWServer розповсюджується під ліцензією **GPLv2 або пізнішою**.

Цей проект ліцензовано на умовах **GNU General Public License version 2** або будь-якої пізнішої версії (GPLv2+).

Ви маєте повну свободу:

- Використовувати програмне забезпечення для будь-яких цілей.
- Вивчати, як працює програма, і адаптувати її до своїх потреб.
- Розповсюджувати копії програмного забезпечення.
- Покращувати програму та публікувати свої покращення.

Повний текст ліцензії доступний за посиланням: [http://www.gnu.org/licenses/gpl.html](http://www.gnu.org/licenses/gpl.html).

## Автор

**Ivan Svarkovsky** - [https://github.com/Svarkovsky](https://github.com/Svarkovsky)

Зроблено з ❤️ для людей.


# cWServer - 轻量级多线程Web服务器

[前往英文版本](#cWServer---Lightweight-Multithreaded-Web-Server)  
[前往乌克兰语版本](#cWServer---Легкий-багатопотоковий-веб-сервер)

<br> <div align="center"> <img src="unCs9b5zHjUiwcKv-generated_image.jpg" alt="cWServer Logo" style="width: 300px; height: auto;"> </div> <br>

**cWServer** 是一个**使用POSIX标准库编写的轻量级多线程Web服务器**。它旨在高效地提供静态内容，并采用多线程架构处理并发连接。该服务器主要用于教育目的，展示Web服务器的基本原理，但也可用于简单的静态网站托管任务。代码的简洁性和标准库的使用使其易于理解、修改和移植到各种平台。

## 主要功能

- **提供静态文件：** cWServer高效地提供HTML、CSS、JavaScript、图像、视频和音频等静态文件。
- **多线程架构：** 服务器使用POSIX线程（`pthreads`）在单独的线程中处理每个传入连接，确保在高并发负载下的良好性能。
- **部分内容支持（Content-Range）：** 服务器支持`Content-Range` HTTP头，允许客户端分部分下载文件，这对于大文件或中断的连接非常有用。
- **禁用缓存（Cache-Control: no-cache）：** 默认情况下，服务器禁用客户端缓存，以确保提供最新内容。
- **目录列表：** 如果请求的是目录而不是特定文件，服务器会生成一个HTML页面，列出该目录中的文件。
- **目录列表样式：** 可以选择目录列表的图标样式：
  - **text：** 文本图标，如`[D]`、`[TXT]`、`[IMG]`等。
  - **emoji：** Emoji图标，如`[📂]`、`[📝]`、`[🖼️]`等。
  - **none：** 无图标。
- **受保护的目录查看模式（以前称为Pseudo-FTP）：** 密码保护的目录查看模式。它使用密码作为路径前缀来限制对某些目录的访问。
- **守护进程模式：** 可以在后台作为守护进程运行服务器。
- **详细日志记录：** 服务器将访问日志和错误日志记录到标准错误输出（`stderr`）。
- **URL编码请求处理：** 服务器正确处理请求中的URL编码字符。
- **index.html处理：** 当请求目录时，服务器首先在该目录中查找`index.html`文件。如果找到，则提供该文件。如果不存在，则返回目录列表（除非启用了受保护的目录查看模式）。

## 架构

当前服务器版本针对**MIPS架构（74kc, mips16, mdsp, EB）**进行了优化。Makefile包含特定于此架构的编译器和链接器设置，包括：

- `-march=74kc -mips16 -mdsp`：GCC选项，用于优化MIPS 74kc架构，使用MIPS16和DSP指令。
- `-EB`：指定大端字节序（如果需要）。
- 特定的链接和strip选项，以减少可执行文件的大小。

**为其他架构构建：**

要为其他架构构建，您可能需要修改`Makefile`中的`CFLAGS`和`LDFLAGS`变量。具体来说，您需要调整：

- `-march=...`：指定目标架构。
- `-mips16 -mdsp -EB`：如果您的架构不支持这些选项或不需要它们，请删除或修改。
- `--sysroot=... -I...`：检查并更新系统include和lib目录的路径（如果它们在您的系统中不同）。

**重要提示：** 用户可以根据GPLv2或更高版本的许可条款自由修改和分发服务器代码。

## 入门

### 编译

要编译服务器，您需要C编译器（例如GCC）和`make`。

1. 确保已安装必要的开发工具（GCC、make和其他必需的库）。
2. 将`cwserver_v0.1a.c`和`Makefile`文件保存在同一目录中。
3. 在该目录中打开终端。
4. 运行`make`命令：

    ```bash
    make
    ```

    这将创建一个名为`cwserver`的可执行文件。

### 运行

要运行服务器，请使用以下命令：

```bash
./cwserver [选项]
```

### 选项

- **`-p port`**  
  指定要监听的端口。使用此选项设置Web服务器等待传入连接的端口。  
  **默认值：** `8080`  
  ```bash
  ./cwserver -p 3000
  ```
- **`-w web_root`**  
  指定Web服务器的根目录。设置文件系统上的目录，服务器将从该目录为客户端提供文件。  
  **默认值：** 当前目录（`.`）  
  ```bash
  ./cwserver -w /var/www/html
  ```
- **`-d`**  
  以守护进程模式（后台模式）运行服务器。使用此选项时，服务器将在后台运行，与终端分离。这对于长期运行服务器非常有用。  
  ```bash
  ./cwserver -d
  ```
- **`-h`**  
  显示帮助信息，描述所有可用选项。使用此选项将打印所有可用命令行选项的帮助信息并终止服务器。  
  ```bash
  ./cwserver -h
  ```
- **`-v`**  
  显示服务器版本信息。使用`-v`选项将打印`cwServer`的版本信息并终止服务器。  
  ```bash
  ./cwserver -v
  ```
- **`-i icon_style`**  
  指定目录列表的图标样式。允许选择HTML目录列表中显示的图标样式。  
  **可能的值：** `text`、`emoji`、`none`  
  **默认值：** `text`  
  ```bash
  ./cwserver -i emoji
  ```
- **`-f ftp_password`**  
  启用Pseudo-FTP模式，使用密码`ftp_password`。激活使用密码作为路径前缀的限制目录访问模式。  
  ```bash
  ./cwserver -f mypassword
  ```

## 使用示例

1. 在端口`3000`上运行服务器，根目录为`/var/www/html`：  
   ```bash
   ./cwserver -p 3000 -w /var/www/html
   ```

2. 以守护进程模式运行服务器，使用`emoji`图标样式：  
   ```bash
   ./cwserver -d -i emoji
   ```

3. 显示帮助信息：  
   ```bash
   ./cwserver -h
   ```

4. 检查服务器版本：  
   ```bash
   ./cwserver -v
   ```

## 许可证

cWServer根据**GPLv2或更高版本**的许可证分发。

该项目根据**GNU通用公共许可证第2版**或任何更高版本（GPLv2+）的条款获得许可。

您拥有以下全部自由：

- 出于任何目的使用软件。
- 研究软件的工作原理并适应您的需求。
- 分发软件的副本。
- 改进软件并发布您的改进。

完整的许可证文本可在以下链接找到：[http://www.gnu.org/licenses/gpl.html](http://www.gnu.org/licenses/gpl.html)。

## 作者

**Ivan Svarkovsky** - [https://github.com/Svarkovsky](https://github.com/Svarkovsky)

用心为人们打造。❤️
