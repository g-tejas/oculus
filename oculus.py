#!/usr/bin/env python3

import sys
import subprocess
import json
import time
from pathlib import Path
import fasteners

def get_windows() -> json:
  result = subprocess.run(["yabai", "-m", "query", "--windows"], capture_output=True)
  return json.loads(result.stdout.decode("utf-8"))

def get_lock_path():
    return Path.home() / '.oculus_sessions.lock'

def load_sessions() -> dict:
    sessions_file = Path.home() / '.oculus_sessions.json'
    lock = fasteners.InterProcessLock(get_lock_path())
    with lock:
        if not sessions_file.exists():
            return {"current_session": None, "sessions": []}
        with open(sessions_file) as f:
            return json.load(f)

def save_sessions(sessions_data: dict):
    sessions_file = Path.home() / '.oculus_sessions.json'
    lock = fasteners.InterProcessLock(get_lock_path())
    with lock:
        with open(sessions_file, 'w') as f:
            json.dump(sessions_data, f, indent=2)

def update_session(window_id: int):
    # Special case: -1 just closes the current session
    lock = fasteners.InterProcessLock(get_lock_path())
    lock.acquire()
    current_time = time.time()
    sessions_data = load_sessions()
    
    if sessions_data["current_session"]:
        current = sessions_data["current_session"]
        duration = current_time - current["start_time"]
        
        sessions_data["sessions"].append({
            "window_id": current["window_id"],
            "app": current["app"],
            "title": current["title"],
            "start_time": current["start_time"],
            "end_time": current_time,
            "duration": duration
        })
    
        sessions_data["current_session"] = None
        
    if window_id == -1:
        return

    windows = get_windows()
    # find the window with the given id
    window = next((w for w in windows if w["id"] == window_id), None)
    if window is None:
        print(f"Window with id {window_id} not found")
        return
        
    save_sessions(sessions_data)
    lock.release()
    
    if window_id == -1:
        sessions_data = load_sessions()
        current_time = time.time()
        
        if sessions_data["current_session"]:
            current = sessions_data["current_session"]
            duration = current_time - current["start_time"]
            
            # Save the completed session
            sessions_data["sessions"].append({
                "window_id": current["window_id"],
                "app": current["app"],
                "title": current["title"],
                "start_time": current["start_time"],
                "end_time": current_time,
                "duration": duration
            })
            # Clear current session
            sessions_data["current_session"] = None
            save_sessions(sessions_data)
        return

    windows = get_windows()
    # find the window with the given id
    window = next((w for w in windows if w["id"] == window_id), None)
    if window is None:
        print(f"Window with id {window_id} not found")
        return
    
    sessions_data = load_sessions()
    current_time = time.time()
    
    # If there's an active session, close it and save it
    if sessions_data["current_session"]:
        current = sessions_data["current_session"]
        duration = current_time - current["start_time"]
        
        # Save the completed session
        sessions_data["sessions"].append({
            "window_id": current["window_id"],
            "app": current["app"],
            "title": current["title"],
            "start_time": current["start_time"],
            "end_time": current_time,
            "duration": duration
        })
    
    # Start new session
    sessions_data["current_session"] = {
        "window_id": window_id,
        "app": window["app"],
        "title": window["title"],
        "start_time": current_time
    }
    
    save_sessions(sessions_data)

if __name__ == "__main__":
  if len(sys.argv) != 2:
    print("Usage: oculus.py <window_id>")
    sys.exit(1)
  window_id = int(sys.argv[1])
  update_session(window_id)

