# 📸 Screenshot Instructions

## 🌐 **Taking Screenshots of the Web Interface**

### **Current Running Servers:**
- **React Web Interface**: http://localhost:3000 (Main dashboard)
- **Python API Server**: http://localhost:5000 (Enhanced server with simulator)

### **Screenshot Locations Needed:**

1. **Main Dashboard** (`dashboard-screenshot.png`)
   - **URL**: http://localhost:3000
   - **Shows**: TricorderFarmDashboard with device cards, professional Material-UI interface
   - **Save as**: `dashboard-screenshot.png` in root directory

2. **OTA Update Wizard** (`ota-update-screenshot.png`) - Optional
   - **URL**: http://localhost:3000 → Click "Update" button on any device
   - **Shows**: 4-step firmware update wizard
   - **Save as**: `docs/images/ota-update-screenshot.png`

3. **ESP32 Simulator** (`simulator-screenshot.png`) - Optional
   - **URL**: http://localhost:5000/simulator 
   - **Shows**: ESP32 display simulator with controls
   - **Save as**: `docs/images/simulator-screenshot.png`

### **How to Take Screenshots:**

#### **Method 1: Windows Snipping Tool (Recommended)**
1. Press `Windows + Shift + S`
2. Select area to capture
3. Screenshot goes to clipboard
4. Open Paint/GIMP and paste (`Ctrl + V`)
5. Save as PNG in the specified location

#### **Method 2: Full Screen Capture**
1. Press `PrtScn` (Print Screen)
2. Open Paint/GIMP and paste (`Ctrl + V`)
3. Crop to desired area
4. Save as PNG

#### **Method 3: Browser Built-in (Chrome/Edge)**
1. Press `F12` to open Developer Tools
2. Press `Ctrl + Shift + P`
3. Type "screenshot" and select "Capture full size screenshot"
4. Save the downloaded file

### **Screenshot Requirements:**
- **Format**: PNG (preferred) or JPG
- **Size**: Minimum 1200px wide for good visibility
- **Quality**: High resolution, clear text
- **Content**: Show key features like device cards, update buttons, professional UI

### **What to Capture:**

#### **Main Dashboard Screenshot Should Show:**
- ✅ **Device Cards**: Multiple tricorder devices with status
- ✅ **Professional UI**: Material-UI design with proper styling
- ✅ **Update Buttons**: Firmware update capabilities visible
- ✅ **Status Indicators**: Online/offline status, battery levels
- ✅ **Navigation**: Header with system title and controls
- ✅ **Responsive Design**: Well-laid-out card grid

#### **Good Screenshot Tips:**
- **Full Window**: Capture entire browser window or app area
- **Clean Background**: Close unnecessary browser tabs/windows
- **Zoom Level**: Use 100% zoom for accurate representation
- **Lighting**: Ensure screen is clearly visible without glare

### **After Taking Screenshots:**

1. **Save in correct location** as specified above
2. **Check file size** - should be under 2MB for README
3. **Verify image quality** - text should be readable
4. **Update README** - the image reference is already added

### **Alternative if Screenshots Don't Work:**

If you can't take screenshots right now, I can:
1. **Create a placeholder** image with system architecture
2. **Add screenshots later** when convenient
3. **Use text-based feature highlights** instead

The README is already prepared to show the screenshot once you save it as `dashboard-screenshot.png` in the root directory!

---

## 🎯 **Priority: Main Dashboard Screenshot**

**Most Important**: Take a screenshot of http://localhost:3000 showing the TricorderFarmDashboard with device cards and save it as `dashboard-screenshot.png` in the root directory. This will immediately enhance the README and show off the professional interface you built!
