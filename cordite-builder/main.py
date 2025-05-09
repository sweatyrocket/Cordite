import tkinter as tk
import asyncio
import discord
from ui import create_ui, create_setup_panel, create_beacon_panel, create_config_panel
from discord_utils import connect_bot, setup_channels
from build_utils import build_solution
from utils import make_draggable, find_msbuild_path
import logging
import threading
import os
import subprocess
import sys
from PIL import Image, ImageTk

class CorditeBuilder:
    def __init__(self, root):
        self.root = root
        self.root.geometry("900x600")
        self.root.configure(bg="#36393F")
        self.no_window = '--no-window' in sys.argv
        if self.no_window:
            self.root.overrideredirect(True)
            self.root.wm_attributes("-topmost", 1)
        else:
            self.root.title("Cordite C2")

        try:
            logo_image = Image.open("c2_logo.png")
            logo_image = logo_image.resize((64, 64), Image.LANCZOS)
            self.logo_photo = ImageTk.PhotoImage(logo_image)
            self.root.iconphoto(True, self.logo_photo)
        except Exception as e:
            logging.error(f"Failed to load c2_logo.png for window icon: {e}")

        self.intents = discord.Intents.default()
        self.intents.guilds = True
        self.bot = None
        self.channel_ids = {}
        self.app_id = ""
        self.ready = asyncio.Event()
        self.vs_path = None
        self.msbuild_path = None
        self.solution_file = r"..\cordite.sln"
        self.building = False
        self.bot_token = ""
        self.cmake_path = r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        self.current_panel = None
        self.find_msbuild_path = find_msbuild_path
        self.create_setup_panel = lambda: create_setup_panel(self)
        self.create_beacon_panel = lambda: create_beacon_panel(self)
        self.create_config_panel = lambda: create_config_panel(self)
        self.build_solution = lambda: build_solution(self)

        self.timezone_var = tk.StringVar(value="Europe/Budapest")
        self.timezone_data = {
            "Europe/Budapest": {"offset": -60, "dst": True},
        }
        create_ui(self)
        if self.no_window:
            make_draggable(self.title_bar, self.root)
        self.loop = asyncio.new_event_loop()
        threading.Thread(target=self.run_asyncio_loop, daemon=True).start()

    def run_asyncio_loop(self):
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    async def on_ready(self):
        logging.info(f"Bot is ready: {self.bot.user}")
        self.app_id = str(self.bot.application_id)
        logging.info(f"Application ID retrieved: {self.app_id}")
        self.ready.set()

    def find_cmake_path(self):
        common_paths = [
            r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
            r"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
            r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
            r"C:\Program Files\CMake\bin\cmake.exe",
            r"C:\Program Files (x86)\CMake\bin\cmake.exe"
        ]

        for path in common_paths:
            if os.path.exists(path):
                logging.info(f"Found CMake at common path: {path}")
                return path
            else:
                logging.debug(f"CMake not found at: {path}")

        try:
            result = subprocess.run("where cmake", capture_output=True, text=True, shell=True, check=True)
            paths = result.stdout.strip().splitlines()
            for path in paths:
                if path.lower().endswith("cmake.exe") and os.path.exists(path):
                    logging.info(f"Found CMake in system PATH: {path}")
                    return path
                else:
                    logging.debug(f"Invalid or non-existent CMake path from 'where cmake': {path}")
        except subprocess.CalledProcessError as e:
            logging.error(f"Error running 'where cmake': {e}")

        logging.warning("CMake not found in common paths or system PATH.")
        return None

    def switch_panel(self, panel_name):
        if self.current_panel:
            self.current_panel.destroy()
        self.current_panel = self.channels[panel_name]()
        self.current_panel.pack(fill="both", expand=True)
        if hasattr(self, "channel_header"):
            self.channel_header.config(text=f"# {panel_name.replace('-', ' ')}")
        for widget in self.channel_frame.winfo_children():
            if widget["text"] == f"# {panel_name.replace('-', ' ')}":
                widget.config(fg="#DCDDDE", bg="#35393E")
            elif widget["text"] in ["SETUP", "PAYLOADS", "CONFIGURATION"]:
                continue
            else:
                widget.config(fg="#72767D", bg="#2F3136")

    def save_config(self):
        self.bot_token = self.token_entry.get()
        logging.info(f"Config saved: token={self.bot_token}, app_id={self.app_id}, channel_ids={self.channel_ids}, cmake_path={self.cmake_path}")
        if "config" in self.channels and self.current_panel and self.channel_header["text"] == "# config":
            self.current_panel.destroy()
            self.current_panel = self.create_config_panel()
            self.current_panel.pack(fill="both", expand=True)

    def save_config_from_panel(self):
        new_token = self.config_token_entry.get()
        if not new_token.startswith("*"):
            self.bot_token = new_token

        self.cmake_path = self.cmake_path_entry.get().strip()

        for channel_name, entry in self.channel_entries.items():
            channel_id = entry.get().strip()
            if channel_id:
                self.channel_ids[channel_name] = channel_id
            elif channel_name in self.channel_ids:
                del self.channel_ids[channel_name]

        self.selected_timezone = self.timezone_var.get()
        logging.info(
            f"Config updated from panel: token={self.bot_token}, app_id={self.app_id}, channel_ids={self.channel_ids}, timezone={self.selected_timezone}, cmake_path={self.cmake_path}")
        self.config_status_label.config(text="Configuration saved successfully!")

    async def run_setup(self):
        if not self.bot_token.strip():
            self.status_label.config(text="Error: No bot token provided", fg="#FF0000")
            logging.error("No bot token provided for setup")
            return

        if not await connect_bot(self):
            self.status_label.config(text="Error: Failed to connect bot (invalid token?)", fg="#FF0000")
            return
        try:
            logging.info("Waiting for bot to be ready...")
            await asyncio.wait_for(self.ready.wait(), timeout=10)
            guilds = self.bot.guilds
            if not guilds:
                logging.error("Bot is not in any servers. Please invite the bot to a server first.")
                self.status_label.config(text="Error: Bot is not in any servers. Invite it first.", fg="#FF0000")
                return
            guild = guilds[0]
            logging.info(f"Using guild: {guild.name} (ID: {guild.id})")
            await setup_channels(self, guild)
            self.save_config()

            cmake_path = self.find_cmake_path()
            if cmake_path:
                self.cmake_path = cmake_path
                self.status_label.config(text=f"Channels fetched, CMake path set to {cmake_path}. Check config tab.", fg="#00FF00")
            else:
                self.cmake_path = ""
                self.status_label.config(text="Channels fetched, CMake not found. Please set path in config tab.", fg="#FFFF00")

        except asyncio.TimeoutError:
            logging.error("Bot failed to become ready within 10 seconds")
            self.status_label.config(text="Error: Bot failed to initialize in time", fg="#FF0000")
        except Exception as e:
            logging.error(f"Setup failed: {str(e)}")
            self.status_label.config(text=f"Setup failed: {str(e)}", fg="#FF0000")
        finally:
            if self.bot:
                await self.bot.close()
                self.bot = None
                self.ready.clear()
                logging.info("Bot connection closed")

    def start_setup(self):
        self.bot_token = self.token_entry.get()
        if not self.bot_token.strip():
            self.status_label.config(text="Error: Please enter a bot token", fg="#FF0000")
            return
        self.status_label.config(text="Setting up server...")
        asyncio.run_coroutine_threadsafe(self.run_setup(), self.loop)

    def close_window(self):
        logging.info("Closing application...")
        if self.bot:
            asyncio.run_coroutine_threadsafe(self.bot.close(), self.loop).result()
            self.bot = None
            self.ready.clear()
        self.loop.call_soon_threadsafe(self.loop.stop)
        self.root.destroy()
        logging.info("Application closed.")

def main():
    root = tk.Tk()
    app = CorditeBuilder(root)
    root.mainloop()

if __name__ == "__main__":
    main()