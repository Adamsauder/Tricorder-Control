<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# Tricorder Control System - Copilot Instructions

This is a multi-component embedded systems project for controlling ESP32-based film set props.

## Project Context

- **Target Hardware**: ESP32 microcontrollers with embedded screens and NeoPixel LEDs
- **Programming Languages**: Python (server), C++ (ESP32 firmware), JavaScript/TypeScript (web interface)
- **Key Technologies**: WiFi networking, video playback, LED control, SACN lighting protocol
- **Priority**: Low-latency command response for film set cuing

## Code Generation Guidelines

### ESP32 Firmware
- Use Arduino framework with ESP32 libraries
- Prioritize performance and memory efficiency
- Include proper error handling for WiFi and SD card operations
- Use FreeRTOS tasks for concurrent video/LED operations

### Python Server
- Use FastAPI or Flask for REST API
- Implement WebSocket connections for real-time communication
- Use asyncio for handling multiple device connections
- Include proper logging and error handling

### Web Interface
- Use modern JavaScript frameworks (React/Vue.js recommended)
- Implement responsive design for mobile/tablet use
- Include real-time updates via WebSocket
- Prioritize usability for film set operators

### Networking
- Use UDP for low-latency commands
- Implement auto-discovery using mDNS/Bonjour
- Include connection retry and failover mechanisms
- Support SACN (E1.31) lighting protocol

## File Organization
- `/firmware/` - ESP32 Arduino sketches and libraries
- `/server/` - Python backend server code
- `/web/` - Frontend web application
- `/docs/` - Technical documentation and specifications
- `/hardware/` - Schematics and hardware documentation

## Performance Requirements
- Command latency < 50ms
- Support for 20+ concurrent device connections
- Efficient video file streaming from SD cards
- Smooth LED animations and transitions

You can find more info and examples at https://modelcontextprotocol.io/llms-full.txt
