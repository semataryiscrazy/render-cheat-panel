#!/usr/bin/env python3
"""
Auth Bot — Setup Script
Run this to configure your Discord bot before starting.

Usage:
    python setup.py
"""

import json
import os
import sys

CONFIG_FILE = os.path.join(os.path.dirname(__file__), "config.json")
ENV_FILE = os.path.join(os.path.dirname(__file__), "..", ".env")

def load_config():
    with open(CONFIG_FILE, "r") as f:
        return json.load(f)

def save_config(cfg):
    with open(CONFIG_FILE, "w") as f:
        json.dump(cfg, f, indent=4)
    print(f"[OK] config.json saved")

def main():
    print("=" * 50)
    print("  Auth Bot — Configuration Setup")
    print("=" * 50)
    print()

    cfg = load_config()
    
    # Check current state
    token = cfg.get("token", "")
    webhook = cfg.get("webhook_url", "")
    
    if token and token != "DISCORD_TOKEN_AQUI":
        print(f"[*] Discord token already configured: {token[:12]}...")
    else:
        print("[!] Discord token not configured.")
        token = input("  Enter your Discord Bot Token: ").strip()
        if token:
            cfg["token"] = token
    
    if webhook and webhook != "WEBHOOK_URL_AQUI":
        print(f"[*] Webhook URL already configured: {webhook[:30]}...")
    else:
        print()
        print("[!] Webhook URL not configured (optional for notifications).")
        webhook = input("  Enter Discord Webhook URL (or leave blank): ").strip()
        if webhook:
            cfg["webhook_url"] = webhook
    
    print()
    print(f"[*] API Port: {cfg.get('api_port', 5000)}")
    port = input(f"  Enter API port (or leave blank for {cfg.get('api_port', 5000)}): ").strip()
    if port:
        try:
            cfg["api_port"] = int(port)
        except ValueError:
            print("  [!] Invalid port, keeping default.")
    
    print()
    print("[*] Admin IDs (comma-separated Discord user IDs):")
    admins = cfg.get("admin_ids", [])
    if admins:
        print(f"  Current: {admins}")
    admin_input = input("  Enter admin IDs (or leave blank): ").strip()
    if admin_input:
        cfg["admin_ids"] = [int(x.strip()) for x in admin_input.split(",") if x.strip().isdigit()]
    
    save_config(cfg)
    
    print()
    print("  Next steps:")
    print("    1. Install dependencies: pip install -r requirements.txt")
    print("    2. Run the bot: python auth_bot.py")
    print()
    print("  Deploy on Railway:")
    print("    - Connect your GitHub repo")
    print("    - Set environment variables (DISCORD_TOKEN, etc.)")
    print("    - Railway will use the Procfile and railway.json")
    print()

if __name__ == "__main__":
    main()
