// МикроББокс - Система управления
class MicroBoxController {
    constructor() {
        this.deviceType = 'unknown';
        this.controlMode = 'tank';
        this.effectMode = 'normal';
        this.isConnected = false;
        this.vrEnabled = false;
        this.gamepadIndex = -1;
        
        // Состояние стиков
        this.leftJoystick = { x: 0, y: 0, active: false };
        this.rightJoystick = { x: 0, y: 0, active: false };
        
        // Состояние клавиш
        this.keyStates = {};
        
        // Настройки
        this.speedSensitivity = 80;
        this.turnSensitivity = 70;
        
        // WebXR для VR
        this.xrSession = null;
        this.controllers = [];
        
        this.init();
    }

    async init() {
        console.log('Инициализация МикроББокс контроллера...');
        
        // Определение типа устройства
        this.detectDeviceType();
        
        // Настройка интерфейса
        this.setupInterface();
        
        // Настройка камеры
        this.setupCameraStream();
        
        // Настройка событий
        this.setupEventListeners();
        
        // Проверка VR поддержки
        await this.checkVRSupport();
        
        // Запуск основного цикла
        this.startMainLoop();
        
        // Скрыть экран загрузки
        setTimeout(() => {
            document.getElementById('loading').classList.add('hidden');
            document.getElementById('mainInterface').classList.remove('hidden');
        }, 2000);
        
        console.log('Инициализация завершена');
    }

    setupCameraStream() {
        const streamImg = document.getElementById('cameraStream');
        if (streamImg) {
            // Используем порт 81 для камеры-сервера
            const streamUrl = window.location.protocol + '//' + window.location.hostname + ':81/stream';
            streamImg.src = streamUrl;
            console.log('Камера стрим:', streamUrl);
        }
    }


    detectDeviceType() {
        const isMobile = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
        const isTablet = /iPad|Android/i.test(navigator.userAgent) && window.innerWidth > 768;
        const isVR = navigator.xr !== undefined;
        
        if (isVR && navigator.xr.isSessionSupported) {
            this.deviceType = 'vr';
        } else if (isMobile && !isTablet) {
            this.deviceType = 'mobile';
        } else if (isTablet) {
            this.deviceType = 'tablet';
        } else {
            this.deviceType = 'desktop';
        }
        
        document.getElementById('deviceType').textContent = this.getDeviceTypeText();
        console.log('Тип устройства:', this.deviceType);
    }

    getDeviceTypeText() {
        switch (this.deviceType) {
            case 'mobile': return '📱 Мобильный';
            case 'tablet': return '📱 Планшет';
            case 'desktop': return '🖥️ ПК';
            case 'vr': return '🥽 VR';
            default: return '❓ Неизвестно';
        }
    }

    setupInterface() {
        // Показать соответствующий интерфейс
        const pcControls = document.getElementById('pcControls');
        const mobileControls = document.getElementById('mobileControls');
        const vrControls = document.getElementById('vrControls');

        // Скрыть все
        pcControls.classList.add('hidden');
        mobileControls.classList.add('hidden');
        vrControls.classList.add('hidden');

        // Показать нужный
        switch (this.deviceType) {
            case 'desktop':
                pcControls.classList.remove('hidden');
                break;
            case 'mobile':
            case 'tablet':
                mobileControls.classList.remove('hidden');
                this.setupMobileJoysticks();
                break;
            case 'vr':
                vrControls.classList.remove('hidden');
                break;
        }
    }

    setupEventListeners() {
        // Fullscreen кнопка
        const fullscreenBtn = document.getElementById('fullscreenBtn');
        if (fullscreenBtn) {
            fullscreenBtn.addEventListener('click', () => this.toggleFullscreen());
        }
        
        // Клавиатура для ПК
        if (this.deviceType === 'desktop') {
            document.addEventListener('keydown', (e) => this.handleKeyDown(e));
            document.addEventListener('keyup', (e) => this.handleKeyUp(e));
            
            // Кнопки управления
            document.querySelectorAll('.control-btn').forEach(btn => {
                btn.addEventListener('mousedown', (e) => this.handleControlButton(e, true));
                btn.addEventListener('mouseup', (e) => this.handleControlButton(e, false));
                btn.addEventListener('mouseleave', (e) => this.handleControlButton(e, false));
            });
        }

        // Геймпады
        window.addEventListener('gamepadconnected', (e) => this.handleGamepadConnected(e));
        window.addEventListener('gamepaddisconnected', (e) => this.handleGamepadDisconnected(e));

        // Селекторы режимов
        const controlModeSelect = document.getElementById('controlMode');
        const effectModeSelect = document.getElementById('effectMode');
        
        if (controlModeSelect) {
            controlModeSelect.addEventListener('change', (e) => {
                this.controlMode = e.target.value;
                this.sendCommand('setControlMode', { mode: this.controlMode });
            });
        }
        
        if (effectModeSelect) {
            effectModeSelect.addEventListener('change', (e) => {
                this.effectMode = e.target.value;
                this.sendCommand('setEffectMode', { mode: this.effectMode });
            });
        }

        // Дополнительные кнопки
        const flashlightBtn = document.getElementById('flashlightBtn') || document.getElementById('mobileFlashlight');
        const hornBtn = document.getElementById('hornBtn') || document.getElementById('mobileHorn');
        const updateBtn = document.getElementById('updateBtn') || document.getElementById('mobileUpdate');
        
        if (flashlightBtn) {
            flashlightBtn.addEventListener('click', () => this.toggleFlashlight());
        }
        
        if (hornBtn) {
            hornBtn.addEventListener('mousedown', () => this.startHorn());
            hornBtn.addEventListener('mouseup', () => this.stopHorn());
            hornBtn.addEventListener('touchstart', () => this.startHorn());
            hornBtn.addEventListener('touchend', () => this.stopHorn());
        }
        
        if (updateBtn) {
            updateBtn.addEventListener('click', () => this.enterUpdateMode());
        }

        // Настройки
        const settingsBtn = document.getElementById('mobileSettings');
        if (settingsBtn) {
            settingsBtn.addEventListener('click', () => this.showSettings());
        }

        // Модальные окна
        this.setupModalHandlers();
    }

    setupMobileJoysticks() {
        const leftJoystick = document.getElementById('leftJoystick');
        const rightJoystick = document.getElementById('rightJoystick');
        
        this.setupJoystick(leftJoystick, 'left');
        this.setupJoystick(rightJoystick, 'right');
    }

    setupJoystick(element, side) {
        let isDragging = false;
        let startX, startY;
        const knob = element.querySelector('.joystick-knob');
        const maxDistance = 40; // Максимальное расстояние от центра

        const handleStart = (e) => {
            isDragging = true;
            element.classList.add('active');
            
            const rect = element.getBoundingClientRect();
            const centerX = rect.left + rect.width / 2;
            const centerY = rect.top + rect.height / 2;
            
            startX = centerX;
            startY = centerY;
            
            e.preventDefault();
        };

        const handleMove = (e) => {
            if (!isDragging) return;
            
            const clientX = e.clientX || (e.touches && e.touches[0].clientX);
            const clientY = e.clientY || (e.touches && e.touches[0].clientY);
            
            const deltaX = clientX - startX;
            const deltaY = clientY - startY;
            const distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);
            
            let x = deltaX;
            let y = deltaY;
            
            if (distance > maxDistance) {
                const angle = Math.atan2(deltaY, deltaX);
                x = Math.cos(angle) * maxDistance;
                y = Math.sin(angle) * maxDistance;
            }

            knob.style.transform = 'translate(' + x + 'px, ' + y + 'px)';

            // Нормализация значений (-1 до 1)
            const normalizedX = x / maxDistance;
            const normalizedY = -y / maxDistance; // Инвертируем Y
            
            if (side === 'left') {
                this.leftJoystick = { x: normalizedX, y: normalizedY, active: true };
            } else {
                this.rightJoystick = { x: normalizedX, y: normalizedY, active: true };
            }
            
            this.updateMovement();
        };

        const handleEnd = () => {
            if (!isDragging) return;
            
            isDragging = false;
            element.classList.remove('active');
            knob.style.transform = 'translate(0px, 0px)';
            
            if (side === 'left') {
                this.leftJoystick = { x: 0, y: 0, active: false };
            } else {
                this.rightJoystick = { x: 0, y: 0, active: false };
            }
            
            this.updateMovement();
        };

        // Мышь события
        element.addEventListener('mousedown', handleStart);
        document.addEventListener('mousemove', handleMove);
        document.addEventListener('mouseup', handleEnd);

        // Сенсорные события
        element.addEventListener('touchstart', handleStart);
        document.addEventListener('touchmove', handleMove);
        document.addEventListener('touchend', handleEnd);
    }

    handleKeyDown(e) {
        this.keyStates[e.code] = true;
        this.updateMovementFromKeyboard();
        
        // Дополнительные клавиши
        switch (e.code) {
            case 'Space':
                e.preventDefault();
                this.startHorn();
                break;
            case 'KeyF':
                e.preventDefault();
                this.toggleFlashlight();
                break;
            case 'Digit1':
            case 'Digit2':
            case 'Digit3':
            case 'Digit4':
                e.preventDefault();
                this.setEffectMode(parseInt(e.code.slice(-1)) - 1);
                break;
        }
    }

    handleKeyUp(e) {
        this.keyStates[e.code] = false;
        this.updateMovementFromKeyboard();
        
        if (e.code === 'Space') {
            e.preventDefault();
            this.stopHorn();
        }
    }

    updateMovementFromKeyboard() {
        let leftSpeed = 0;
        let rightSpeed = 0;
        
        // WASD или стрелки
        const forward = this.keyStates['KeyW'] || this.keyStates['ArrowUp'];
        const backward = this.keyStates['KeyS'] || this.keyStates['ArrowDown'];
        const left = this.keyStates['KeyA'] || this.keyStates['ArrowLeft'];
        const right = this.keyStates['KeyD'] || this.keyStates['ArrowRight'];
        
        if (forward) {
            leftSpeed = this.speedSensitivity;
            rightSpeed = this.speedSensitivity;
        }
        
        if (backward) {
            leftSpeed = -this.speedSensitivity;
            rightSpeed = -this.speedSensitivity;
        }
        
        if (left) {
            leftSpeed -= this.turnSensitivity;
            rightSpeed += this.turnSensitivity;
        }
        
        if (right) {
            leftSpeed += this.turnSensitivity;
            rightSpeed -= this.turnSensitivity;
        }
        
        // Ограничение значений
        leftSpeed = Math.max(-100, Math.min(100, leftSpeed));
        rightSpeed = Math.max(-100, Math.min(100, rightSpeed));
        
        this.sendMovementCommand(leftSpeed, rightSpeed);
    }

    handleControlButton(e, pressed) {
        const direction = e.target.dataset.direction;
        
        if (pressed) {
            e.target.classList.add('active');
            
            switch (direction) {
                case 'forward':
                    this.sendMovementCommand(this.speedSensitivity, this.speedSensitivity);
                    break;
                case 'backward':
                    this.sendMovementCommand(-this.speedSensitivity, -this.speedSensitivity);
                    break;
                case 'left':
                    this.sendMovementCommand(-this.turnSensitivity, this.turnSensitivity);
                    break;
                case 'right':
                    this.sendMovementCommand(this.turnSensitivity, -this.turnSensitivity);
                    break;
                case 'stop':
                    this.sendMovementCommand(0, 0);
                    break;
            }
        } else {
            e.target.classList.remove('active');
            if (direction !== 'stop') {
                this.sendMovementCommand(0, 0);
            }
        }
    }

    updateMovement() {
        let leftSpeed = 0;
        let rightSpeed = 0;
        
        if (this.controlMode === 'tank') {
            // Танковый режим: левый стик = левая сторона, правый стик = правая сторона
            leftSpeed = this.leftJoystick.y * this.speedSensitivity;
            rightSpeed = this.rightJoystick.y * this.speedSensitivity;
        } else if (this.controlMode === 'differential') {
            // Дифференциальный: правый стик = скорость, левый стик = поворот
            const speed = this.rightJoystick.y * this.speedSensitivity;
            const turn = this.leftJoystick.x * this.turnSensitivity;
            
            leftSpeed = speed - turn;
            rightSpeed = speed + turn;
        }
        
        // Ограничение значений
        leftSpeed = Math.max(-100, Math.min(100, leftSpeed));
        rightSpeed = Math.max(-100, Math.min(100, rightSpeed));
        
        this.sendMovementCommand(leftSpeed, rightSpeed);
    }

    sendMovementCommand(leftSpeed, rightSpeed) {
        this.sendCommand('move', {
            left: Math.round(leftSpeed),
            right: Math.round(rightSpeed)
        });
    }

    async sendCommand(command, data = {}) {
        try {
            const response = await fetch('/command', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    command: command,
                    ...data
                })
            });
            
            if (response.ok) {
                this.updateConnectionStatus(true);
                return await response.json();
            } else {
                this.updateConnectionStatus(false);
                console.error('Ошибка команды:', response.statusText);
            }
        } catch (error) {
            this.updateConnectionStatus(false);
            console.error('Ошибка соединения:', error);
        }
    }

    updateConnectionStatus(connected) {
        this.isConnected = connected;
        const indicator = document.getElementById('connectionStatus');
        const text = document.getElementById('connectionText');
        
        if (connected) {
            indicator.classList.add('connected');
            text.textContent = 'Подключено';
        } else {
            indicator.classList.remove('connected');
            text.textContent = 'Нет соединения';
        }
    }

    toggleFlashlight() {
        this.sendCommand('flashlight', { toggle: true });
        
        const btn = document.getElementById('flashlightBtn') || document.getElementById('mobileFlashlight');
        if (btn) {
            btn.classList.toggle('active');
        }
    }

    startHorn() {
        this.sendCommand('horn', { state: true });
        
        const btn = document.getElementById('hornBtn') || document.getElementById('mobileHorn');
        if (btn) {
            btn.classList.add('active');
        }
    }

    stopHorn() {
        this.sendCommand('horn', { state: false });
        
        const btn = document.getElementById('hornBtn') || document.getElementById('mobileHorn');
        if (btn) {
            btn.classList.remove('active');
        }
    }

    setEffectMode(mode) {
        const modes = ['normal', 'police', 'fire', 'ambulance'];
        this.effectMode = modes[mode] || 'normal';
        
        this.sendCommand('setEffectMode', { mode: this.effectMode });
        
        const select = document.getElementById('effectMode');
        if (select) {
            select.value = this.effectMode;
        }
    }

    async checkVRSupport() {
        if (navigator.xr) {
            try {
                const supported = await navigator.xr.isSessionSupported('immersive-vr');
                if (supported) {
                    this.vrEnabled = true;
                    console.log('VR поддерживается');
                    // Можно добавить кнопку для входа в VR
                }
            } catch (error) {
                console.log('VR не поддерживается:', error);
            }
        }
    }

    handleGamepadConnected(e) {
        console.log('Геймпад подключен:', e.gamepad);
        this.gamepadIndex = e.gamepad.index;
    }

    handleGamepadDisconnected(e) {
        console.log('Геймпад отключен:', e.gamepad);
        this.gamepadIndex = -1;
    }

    processGamepad() {
        if (this.gamepadIndex === -1) return;
        
        const gamepad = navigator.getGamepads()[this.gamepadIndex];
        if (!gamepad) return;
        
        // Левый стик - поворот (axes 0, 1)
        // Правый стик - движение (axes 2, 3)
        const leftX = gamepad.axes[0];
        const leftY = gamepad.axes[1];
        const rightX = gamepad.axes[2];
        const rightY = gamepad.axes[3];
        
        // Обновляем виртуальные стики
        this.leftJoystick = { x: leftX, y: -leftY, active: Math.abs(leftX) > 0.1 || Math.abs(leftY) > 0.1 };
        this.rightJoystick = { x: rightX, y: -rightY, active: Math.abs(rightX) > 0.1 || Math.abs(rightY) > 0.1 };
        
        this.updateMovement();
        
        // Кнопки
        if (gamepad.buttons[0] && gamepad.buttons[0].pressed) { // A/X
            this.startHorn();
        } else {
            this.stopHorn();
        }
        
        if (gamepad.buttons[1] && gamepad.buttons[1].pressed) { // B/Circle
            this.toggleFlashlight();
        }
    }

    setupModalHandlers() {
        // Модальное окно настроек
        const settingsModal = document.getElementById('settingsModal');
        const closeBtn = settingsModal?.querySelector('.close');
        
        if (closeBtn) {
            closeBtn.addEventListener('click', () => {
                settingsModal.classList.add('hidden');
            });
        }
        
        if (settingsModal) {
            settingsModal.addEventListener('click', (e) => {
                if (e.target === settingsModal) {
                    settingsModal.classList.add('hidden');
                }
            });
        }
        
        // Кнопка сохранения настроек
        const saveBtn = document.getElementById('saveSettings');
        if (saveBtn) {
            saveBtn.addEventListener('click', () => this.saveSettings());
        }
    }

    showSettings() {
        const modal = document.getElementById('settingsModal');
        if (modal) {
            modal.classList.remove('hidden');
            
            // Загрузить текущие настройки
            document.getElementById('speedSensitivity').value = this.speedSensitivity;
            document.getElementById('turnSensitivity').value = this.turnSensitivity;
            
            // Установить текущие режимы
            document.querySelector('input[name="controlMode"][value="' + this.controlMode + '"]').checked = true;
            document.querySelector('input[name="effectMode"][value="' + this.effectMode + '"]').checked = true;
        }
    }

    saveSettings() {
        // Сохранить настройки
        this.speedSensitivity = parseInt(document.getElementById('speedSensitivity').value);
        this.turnSensitivity = parseInt(document.getElementById('turnSensitivity').value);
        
        const controlMode = document.querySelector('input[name="controlMode"]:checked');
        const effectMode = document.querySelector('input[name="effectMode"]:checked');
        
        if (controlMode) {
            this.controlMode = controlMode.value;
            this.sendCommand('setControlMode', { mode: this.controlMode });
        }
        
        if (effectMode) {
            this.effectMode = effectMode.value;
            this.sendCommand('setEffectMode', { mode: this.effectMode });
        }
        
        // Сохранить в localStorage
        localStorage.setItem('microbox-settings', JSON.stringify({
            speedSensitivity: this.speedSensitivity,
            turnSensitivity: this.turnSensitivity,
            controlMode: this.controlMode,
            effectMode: this.effectMode
        }));
        
        // Закрыть модальное окно
        document.getElementById('settingsModal').classList.add('hidden');
        
        console.log('Настройки сохранены');
    }

    enterUpdateMode() {
        // Подтверждение входа в режим обновления
        if (confirm('Вы уверены, что хотите войти в режим обновления прошивки? Это остановит все функции робота.')) {
            this.sendCommand('enterUpdateMode').then(() => {
                // Показать сообщение об успешном входе в режим обновления
                alert('Режим обновления активирован! Робот перезапустится в режиме обновления. Подключитесь к WiFi "МикроББокс-Обновление" и откройте 192.168.4.1');
            });
        }
    }

    loadSettings() {
        const saved = localStorage.getItem('microbox-settings');
        if (saved) {
            try {
                const settings = JSON.parse(saved);
                this.speedSensitivity = settings.speedSensitivity || 80;
                this.turnSensitivity = settings.turnSensitivity || 70;
                this.controlMode = settings.controlMode || 'tank';
                this.effectMode = settings.effectMode || 'normal';
                
                console.log('Настройки загружены');
            } catch (error) {
                console.error('Ошибка загрузки настроек:', error);
            }
        }
    }
    
    toggleFullscreen() {
        if (!document.fullscreenElement && 
            !document.webkitFullscreenElement && 
            !document.mozFullScreenElement && 
            !document.msFullscreenElement) {
            // Войти в fullscreen
            const elem = document.documentElement;
            if (elem.requestFullscreen) {
                elem.requestFullscreen();
            } else if (elem.webkitRequestFullscreen) {
                elem.webkitRequestFullscreen();
            } else if (elem.mozRequestFullScreen) {
                elem.mozRequestFullScreen();
            } else if (elem.msRequestFullscreen) {
                elem.msRequestFullscreen();
            }
            
            // Попытка заблокировать ориентацию в альбомную на мобильных
            if (screen.orientation && screen.orientation.lock) {
                screen.orientation.lock('landscape').catch(err => {
                    console.log('Не удалось заблокировать ориентацию:', err);
                });
            }
        } else {
            // Выйти из fullscreen
            if (document.exitFullscreen) {
                document.exitFullscreen();
            } else if (document.webkitExitFullscreen) {
                document.webkitExitFullscreen();
            } else if (document.mozCancelFullScreen) {
                document.mozCancelFullScreen();
            } else if (document.msExitFullscreen) {
                document.msExitFullscreen();
            }
            
            // Разблокировать ориентацию
            if (screen.orientation && screen.orientation.unlock) {
                screen.orientation.unlock();
            }
        }
    }

    startMainLoop() {
        const loop = () => {
            // Обработка геймпада
            this.processGamepad();
            
            // Проверка соединения
            if (Date.now() % 5000 < 16) { // Каждые 5 секунд
                this.sendCommand('ping');
            }
            
            requestAnimationFrame(loop);
        };
        
        loop();
    }
}

// Запуск при загрузке страницы
document.addEventListener('DOMContentLoaded', () => {
    window.microBoxController = new MicroBoxController();
});

// Предотвращение случайного закрытия
window.addEventListener('beforeunload', (e) => {
    e.preventDefault();
    e.returnValue = '';
});

console.log('МикроББокс система управления загружена');