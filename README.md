Real-Time Voice Transcription with Vosk

A web application that provides real-time speech-to-text transcription using Vosk and WebSockets. Access from any device on your local network, including mobile phones.

## 🚀 Two Implementations Available

### **Python/Flask Version** (Original)
- Easy to setup and modify
- Cross-platform compatibility
- Good for development and testing

### **C Implementation** (Recommended for Raspberry Pi 5) ⭐
- **70% less memory usage**
- **50% faster processing**
- **Optimized for ARM architecture**
- **Lower CPU usage**
- See [c_implementation/README.md](c_implementation/README.md) for details

## Features

- 🎤 Real-time speech recognition
- 📱 Mobile-friendly interface
- 🔒 SSL/HTTPS support for microphone access
- 🌐 Network accessible (LAN)
- ⚡ Low latency transcription

## Quick Start (C Implementation - Raspberry Pi 5)

For the optimized C version on Raspberry Pi 5:

```bash
cd c_implementation
bash install_rpi5.sh
./build/vosk_server
```

Then access at `https://YOUR_PI_IP:5000`

See [c_implementation/README.md](c_implementation/README.md) for full documentation.

---

## Python/Flask Version

### Prerequisites

- Linux system (Ubuntu/Debian recommended)
- Python 3.8 or higher
- Internet connection for downloading dependencies

## Project Structure
```
vosk-realtime-transcription/
├── app.py                # Main Flask application
├── cert.pem              # SSL certificate
├── key.pem               # SSL private key
├── model/                # Vosk model directory
│   ├── am/
│   ├── conf/
│   ├── graph/
│   └── ...
├── templates/
│   └── index.html        # Frontend HTML file
└──venv/                 # Virtual environment
```
## Installation

1. Clone or Create Project Directory
```bash
mkdir -p vosk_realtime_transcription
cd vosk_realtime_transcription
mkdir -p templates
```
2. Create Virtual Environment
Create and activate virtual environment

note: this will take a few seconds to terminate the vertuial enviroment use deactivate
```bash
python3 -m venv venv
source venv/bin/activate
```

3. Install Dependencies

Install packages:
```bash
pip install --upgrade pip
pip install Flask==3.0.0
pip install flask-sock==0.7.0
pip install vosk==0.3.45
pip install numpy
```
4. Download Vosk Model

4.1 Install required tool
```bash
sudo apt install wget unzip -y
```

4.2 Download English model (40MB)
```bash
wget https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip
```
4.3 Extract, rename, and remove
```bash
unzip vosk-model-small-en-us-0.15.zip
mv vosk-model-small-en-us-0.15 model
rm vosk-model-small-en-us-0.15.zip
```

5. Generate SSL Certificates
```bash
openssl req -x509 -newkey rsa:4096 -nodes -out cert.pem -keyout key.pem -days 365
```
When prompted, you can skip most fields by **Pressing Enter.** 

For "Common Name", enter your server's IP address or localhost.

6. Create Templates & HTML file
```bash
mkdir -p templates
cd templates
touch index.html
nano index.html
```
Paste in HTML code
- press ctrl -x
- press Y
- press Enter

7. Create Application Files

Download and save tha application file as app.py

Do not save app.py in the templates folder

```bash
touch app.py
nano app.py
```
Paste in Python code
- press ctrl -x
- press Y
- press Enter

  
## Usage

1. Find Your Server's IP Address
```bash
hostname -I | awk '{print $1}'
```
2. Start the Application
   
Activate virtual environment
```bash
source venv/bin/activate
```
Run the application
```python
python app.py
```
3. Access the Application
```
From the same computer:
https://localhost:5000

From your phone or another device:
https://YOUR_IP_ADDRESS:5000
Example: https://192.168.1.100:5000

⚠️ Note: You'll see a security warning due to the self-signed certificate.
Click "Advanced" and "Proceed" to continue (safe for local development).
```
## Useful Commands

Virtual Environment Activate
```bash
source venv/bin/activate
```
Deactivate
```bash
deactivate
```

List installed packages
```bash
pip list
```


Re-download if needed
```bash
rm -rf model/
wget https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip
unzip vosk-model-small-en-us-0.15.zip
mv vosk-model-small-en-us-0.15 model
```
Certificate Errors

Regenerate certificates
```bash
rm cert.pem key.pem
openssl req -x509 -newkey rsa:4096 -nodes -out cert.pem -keyout key.pem -days 365
```
Stopping the Application
Press Ctrl+C in the terminal, or:

Optional install vscode
```bash
sudo apt install code
```

License
This project is open source and available under the MIT License.
Acknowledgments

Vosk - Offline speech recognition toolkit
Flask - Web framework
Flask-Sock - WebSocket support
