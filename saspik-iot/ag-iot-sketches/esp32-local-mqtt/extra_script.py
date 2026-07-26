"""
PlatformIO extra script: reads .env file and generates env_config.h with defines.

Usage: add to platformio.ini:
  extra_scripts = extra_script.py
"""
import os
from pathlib import Path


def read_env_file(env_path: str) -> dict:
    """Parse a .env file and return key-value pairs."""
    result = {}
    env_file = Path(env_path)
    if not env_file.exists():
        return result
    with open(env_file) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, _, value = line.partition("=")
            key = key.strip()
            value = value.strip().strip('"').strip("'")
            if key and value:
                result[key] = value
    return result


def generate_env_header():
    """Generate src/env_config.h from .env file."""
    env_vars = read_env_file(".env")
    print(f"[extra_script] Found {len(env_vars)} vars in .env: {list(env_vars.keys())}")

    header_path = Path("src/env_config.h")
    with open(header_path, "w") as f:
        f.write("// Auto-generated from .env by extra_script.py\n")
        f.write("// Do not edit manually.\n")
        f.write("#pragma once\n\n")
        for key, value in env_vars.items():
            f.write(f'#ifndef {key}\n')
            f.write(f'    #define {key} "{value}"\n')
            f.write(f'#endif\n')
        print(f"[extra_script] Generated {header_path}")


Import("env")
generate_env_header()