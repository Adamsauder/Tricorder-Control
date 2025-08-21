# Archive Directory

This directory contains files that are no longer actively used in the Prop Control System but are preserved for historical reference.

## Directory Structure

- **`legacy_servers/`** - Previous server implementations that have been superseded by `enhanced_server.py`
- **`test_scripts/`** - Development and testing scripts used during project development
- **`legacy_launchers/`** - Old launcher scripts for archived server versions
- **`development_docs/`** - Historical documentation and progress tracking files
- **`old_servers/`** - Recently archived server files (August 2025)
- **`test_files/`** - Recently archived test and utility scripts (August 2025)

## Why These Files Were Archived

These files were moved to archive to:
1. Reduce clutter in the main project directory
2. Make it easier for new users to understand the current active codebase
3. Preserve development history without interfering with current operations
4. Simplify deployment and maintenance

## Restoration

If you need any of these files for reference or restoration:
1. They can be copied back to their original locations
2. Legacy servers may require additional dependencies
3. Check git history for the original file locations

## Current Active System

The active system now uses:
- `server/enhanced_server.py` - Main server with image display controls
- `server/enhanced_sacn_controller.py` - Enhanced sACN lighting protocol
- `web/enhanced-prop-dashboard.html` - Primary web interface
- `start_enhanced_server.bat/.ps1` - Active launcher scripts
- Clean firmware structure
- Streamlined launchers

## Recent Cleanup (August 21, 2025)

**Files moved to `old_servers/`:**
- `gui_server.py`, `standalone_server.py`, `sacn_controller.py`, `tricorder_service.py`
- Old `templates/` directory (superseded by enhanced dashboard)
- Old startup scripts and README files

**Files moved to `test_files/`:**
- All `test_*.py` and `debug_*.py` files from root and server directories
- Utility scripts like `check_sacn_data.py`, `convert_images.py`
- `quick_test.py` from server directory

**Current clean server directory contains only:**
- `enhanced_server.py` - Active server
- `enhanced_sacn_controller.py` - Active sACN controller
- `requirements.txt` - Dependencies
