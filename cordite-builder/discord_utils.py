import asyncio
import discord
from discord.ext import commands
from tkinter import messagebox
import logging

async def connect_bot(app):
    if not app.bot:
        token = app.token_entry.get()
        if not token:
            messagebox.showerror("Error", "Please enter a bot token")
            return False
        try:
            logging.info("Attempting to connect bot...")
            app.bot = commands.Bot(command_prefix="!", intents=app.intents)
            app.bot.add_listener(app.on_ready)
            asyncio.create_task(app.bot.start(token))
            logging.info("Bot connection initiated")
            return True
        except Exception as e:
            logging.error(f"Bot connection failed: {str(e)}")
            messagebox.showerror("Error", f"Failed to connect bot: {str(e)}")
            return False
    return True

async def setup_channels(app, guild):
    logging.info(f"Setting up channels in guild: {guild.name} (ID: {guild.id})")
    required_channels = ["beacons", "commands", "uploads", "downloads"]
    existing_channels = {ch.name: ch.id for ch in guild.channels if ch.name in required_channels}
    all_exist = all(ch in existing_channels for ch in required_channels)
    if all_exist and not app.wipe_var.get():
        logging.info("All required channels exist, retrieving IDs...")
        app.channel_ids = existing_channels
        for channel_name, channel_id in app.channel_ids.items():
            logging.info(f"Found channel: {channel_name} with ID: {channel_id}")
    else:
        if app.wipe_var.get():
            logging.info("Wiping all existing channels...")
            for channel in guild.channels:
                try:
                    await channel.delete()
                    logging.info(f"Deleted channel: {channel.name} (ID: {channel.id})")
                except discord.errors.Forbidden as e:
                    logging.error(f"Permission denied deleting channel {channel.name}: {str(e)}")
                    raise
                except Exception as e:
                    logging.error(f"Failed to delete channel {channel.name}: {str(e)}")
                    raise
        channels = [
            ("Beacons", [("beacons", "channel")]),
            ("Commands", [("commands", "channel")]),
            ("Exports", [("uploads", "channel"), ("downloads", "channel")])
        ]
        logging.info("Creating new channel structure...")
        for header_name, subchannels in channels:
            try:
                category = await guild.create_category(header_name)
                logging.info(f"Created category: {header_name} (ID: {category.id})")
                for channel_name, _ in subchannels:
                    channel = await guild.create_text_channel(channel_name, category=category)
                    app.channel_ids[channel_name] = channel.id
                    logging.info(f"Created channel: {channel_name} with ID: {channel.id}")
            except Exception as e:
                logging.error(f"Error creating channels under {header_name}: {str(e)}")
                raise