import subprocess
import sys

def install_package(package):
    """Установка Python пакетов через pip."""
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", package])
        print(f"Successfully installed {package}")
        return True
    except subprocess.CalledProcessError:
        print(f"Failed to install {package}")
        return False

def main():
    """Установка необходимых зависимостей для сборки проекта."""
    packages = [
        "htmlmin",
        "jsmin", 
        "csscompressor"
    ]
    
    print("Installing Python dependencies for МикроББокс build...")
    
    for package in packages:
        install_package(package)
    
    print("Dependencies installation completed!")

if __name__ == "__main__":
    main()