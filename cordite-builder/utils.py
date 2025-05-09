import logging
import os
import tkinter as tk

logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

def make_draggable(title_bar, root):
    def start_drag(event):
        root.x = event.x
        root.y = event.y
    def drag(event):
        deltax = event.x - root.x
        deltay = event.y - root.y
        x = root.winfo_x() + deltax
        y = root.winfo_y() + deltay
        root.geometry(f"+{x}+{y}")
    title_bar.bind("<Button-1>", start_drag)
    title_bar.bind("<B1-Motion>", drag)

def find_msbuild_path():
    msbuild_path = r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\amd64\MSBuild.exe"
    if os.path.exists(msbuild_path):
        logging.info(f"Using hardcoded MSBuild path: {msbuild_path}")
        return msbuild_path
    else:
        logging.error("Hardcoded MSBuild.exe not found.")
        return None