#!/usr/bin/env python3
"""
Quick test script to verify server components are working
"""

import sys
import importlib

def test_import(module_name):
    try:
        importlib.import_module(module_name)
        print(f"✅ {module_name} - OK")
        return True
    except ImportError as e:
        print(f"❌ {module_name} - FAILED: {e}")
        return False

def main():
    print("🔍 Testing Tricorder Server Dependencies\n")
    
    modules = [
        "fastapi",
        "uvicorn", 
        "pydantic",
        "redis",
        "aiofiles",
        "websockets",
        "json",
        "sqlite3",
        "asyncio",
        "socket",
        "time",
        "uuid"
    ]
    
    results = []
    for module in modules:
        results.append(test_import(module))
    
    print(f"\n📊 Results: {sum(results)}/{len(results)} modules imported successfully")
    
    if all(results):
        print("\n🎉 All dependencies are working! Server is ready to run.")
        return 0
    else:
        print("\n⚠️  Some dependencies are missing. Check the installation.")
        return 1

if __name__ == "__main__":
    sys.exit(main())
