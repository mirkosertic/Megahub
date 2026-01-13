Import("env")

import os
import subprocess
import shutil

print("=" * 50)
print("Building frontend with Vite...")
print("=" * 50)

project_dir = env.get("PROJECT_DIR")
frontend_dir = os.path.join(project_dir, "frontend")
data_dir = os.path.join(project_dir, "data")

# Check if frontend directory exists
if not os.path.exists(frontend_dir):
    print("Frontend directory not found, skipping build")
    env.Exit(1)

# Install dependencies if needed
node_modules = os.path.join(frontend_dir, "node_modules")
if not os.path.exists(node_modules):
    print("Installing dependencies...")
    if os.name == 'nt':
        subprocess.run(["npm.cmd", "install"], cwd=frontend_dir, check=True)
    else:    
        subprocess.run(["npm", "install"], cwd=frontend_dir, check=True)

# Build with Vite (outputs directly to ../data)
print("Running Vite build...")
if os.name == 'nt':
    result = subprocess.run(
        ["npm.cmd", "run", "build:web"], 
        cwd=frontend_dir, 
        capture_output=True,
        text=True
    )
else:    
    result = subprocess.run(
        ["npm", "run", "build:web"], 
        cwd=frontend_dir, 
        capture_output=True,
        text=True
    )

if result.returncode != 0:
    print("Build failed:")
    print(result.stderr)
    env.Exit(1)

print(result.stdout)

# List built files
if os.path.exists(data_dir):
    print("\nBuilt files:")
    for root, dirs, files in os.walk(data_dir):
        for file in files:
            filepath = os.path.join(root, file)
            size = os.path.getsize(filepath)
            rel_path = os.path.relpath(filepath, data_dir)
            print(f"  {rel_path} ({size} bytes)")

print("=" * 50)
print("Frontend build complete!")
print("=" * 50)
