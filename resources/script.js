// МикроББокс - Система управления
class MicroBoxController {
    constructor() {
        // Конфигурация
        this.GITHUB_REPO = 'GOODWORKRINKZ/microbbox';  // GitHub репозиторий для проверки обновлений
        
        this.deviceType = 'unknown';
        this.controlMode = 'differential';
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
        
        // VR состояния кнопок
        this.vrTriggerPressed = false;
        this.vrGripPressed = false;
        this.vrButtonAPressed = false;
        this.vrButtonBPressed = false;
        this.lastVRLeftSpeed = 0;
        this.lastVRRightSpeed = 0;
        
        // T-800 overlay state
        this.t800Interval = null;
        this.t800StartTime = null;
        
        // Throttling для команд движения
        this.lastMovementTime = 0;
        this.lastLeftSpeed = 0;
        this.lastRightSpeed = 0;
        this.movementThrottle = 50; // мс между командами движения
        
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
        
        // Check for updates on startup (after a delay)
        setTimeout(() => {
            this.checkForUpdatesOnStartup();
        }, 5000);
        
        console.log('Инициализация завершена');
    }
    
    async checkForUpdatesOnStartup() {
        try {
            // Check if auto-update is enabled and don't offer is not set
            const settingsResponse = await fetch('/api/update/settings');
            if (!settingsResponse.ok) return;
            
            const settings = await settingsResponse.json();
            if (!settings.autoUpdate || settings.dontOffer) {
                return;
            }
            
            // Check for updates
            const response = await fetch('/api/update/check');
            if (response.ok) {
                const data = await response.json();
                
                if (data.hasUpdate) {
                    const message = `Доступна новая версия прошивки!\n\nНовая версия: ${data.version}\nРелиз: ${data.releaseName}\n\nОткрыть настройки для обновления?`;
                    if (confirm(message)) {
                        this.showSettings();
                        // Scroll to update section
                        setTimeout(() => {
                            const updateSection = document.querySelector('.update-info');
                            if (updateSection) {
                                updateSection.scrollIntoView({ behavior: 'smooth', block: 'center' });
                            }
                            // Also check for updates to show the new version info
                            this.checkForUpdates();
                        }, 500);
                    }
                }
            }
        } catch (error) {
            console.log('Could not check for updates on startup:', error);
        }
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
        const isOculusBrowser = /OculusBrowser/i.test(navigator.userAgent);
        
        // Если это Oculus браузер, не определяем как VR автоматически
        // Пользователь должен нажать кнопку входа в VR
        if (isOculusBrowser || isMobile) {
            this.deviceType = 'mobile';
        } else if (isTablet) {
            this.deviceType = 'tablet';
        } else {
            this.deviceType = 'desktop';
        }
        
        document.getElementById('deviceType').textContent = this.getDeviceTypeText();
        console.log('Тип устройства:', this.deviceType);
        console.log('Oculus Browser:', isOculusBrowser);
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

        // Показать нужный в зависимости от активной сессии
        // VR контролы показываются только когда активна VR сессия
        switch (this.deviceType) {
            case 'desktop':
                pcControls.classList.remove('hidden');
                break;
            case 'mobile':
            case 'tablet':
                mobileControls.classList.remove('hidden');
                this.setupMobileJoysticks();
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

        // Селектор эффектов
        const effectModeSelect = document.getElementById('effectMode');
        
        if (effectModeSelect) {
            effectModeSelect.addEventListener('change', (e) => {
                this.effectMode = e.target.value;
                this.sendCommand('setEffectMode', { mode: this.effectMode });
                this.updateT800Overlay();
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
            updateBtn.addEventListener('click', () => {
                this.showSettings();
                // Scroll to update section
                setTimeout(() => {
                    const updateSection = document.querySelector('.update-info');
                    if (updateSection) {
                        updateSection.scrollIntoView({ behavior: 'smooth', block: 'center' });
                    }
                }, 100);
            });
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
            
            let deltaX = clientX - startX;
            let deltaY = clientY - startY;
            
            // Ограничение движения джойстиков для дифференциального режима
            if (side === 'left') {
                // Левый джойстик: только горизонтальное движение (поворот)
                deltaY = 0;
                deltaX = Math.max(-maxDistance, Math.min(maxDistance, deltaX));
            } else {
                // Правый джойстик: только вертикальное движение (скорость)
                deltaX = 0;
                deltaY = Math.max(-maxDistance, Math.min(maxDistance, deltaY));
            }
            
            let x = deltaX;
            let y = deltaY;

            knob.style.transform = 'translate(' + x + 'px, ' + y + 'px)';

            // Нормализация значений (-1 до 1)
            const normalizedX = x / maxDistance;
            const normalizedY = -y / maxDistance; // Инвертируем Y
            
            if (side === 'left') {
                this.leftJoystick = { x: normalizedX, y: 0, active: true };
            } else {
                this.rightJoystick = { x: 0, y: normalizedY, active: true };
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
        
        // Дифференциальный режим: правый стик = скорость (только вертикаль), левый стик = поворот (только горизонталь)
        // Формула соответствует ROS diff_drive_controller и ArduPilot:
        // left_motor = speed + turn (поворот вправо: левый быстрее)
        // right_motor = speed - turn (поворот вправо: правый медленнее)
        const speed = this.rightJoystick.y * this.speedSensitivity;
        const turn = this.leftJoystick.x * this.turnSensitivity;
        
        leftSpeed = speed + turn;
        rightSpeed = speed - turn;
        
        // Ограничение значений
        leftSpeed = Math.max(-100, Math.min(100, leftSpeed));
        rightSpeed = Math.max(-100, Math.min(100, rightSpeed));
        
        this.sendMovementCommand(leftSpeed, rightSpeed);
    }

    sendMovementCommand(leftSpeed, rightSpeed) {
        // Throttling: отправляем только если прошло достаточно времени
        // или значения значительно изменились
        const now = Date.now();
        const timeSinceLastSend = now - this.lastMovementTime;
        const speedChanged = Math.abs(leftSpeed - this.lastLeftSpeed) > 5 || 
                           Math.abs(rightSpeed - this.lastRightSpeed) > 5;
        
        // Отправляем если прошло >= 50ms ИЛИ скорость изменилась на >5%
        if (timeSinceLastSend >= this.movementThrottle || speedChanged) {
            this.sendCommand('move', {
                left: Math.round(leftSpeed),
                right: Math.round(rightSpeed)
            });
            
            this.lastMovementTime = now;
            this.lastLeftSpeed = leftSpeed;
            this.lastRightSpeed = rightSpeed;
        }
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
        const modes = ['normal', 'police', 'fire', 'ambulance', 'terminator'];
        this.effectMode = modes[mode] || 'normal';
        
        this.sendCommand('setEffectMode', { mode: this.effectMode });
        
        const select = document.getElementById('effectMode');
        if (select) {
            select.value = this.effectMode;
        }
        
        // Handle T-800 overlay
        this.updateT800Overlay();
    }
    
    updateT800Overlay() {
        const overlay = document.getElementById('t800Overlay');
        if (!overlay) return;
        
        if (this.effectMode === 'terminator') {
            // Show T-800 overlay
            overlay.classList.remove('hidden');
            this.startT800Updates();
        } else {
            // Hide T-800 overlay
            overlay.classList.add('hidden');
            this.stopT800Updates();
        }
    }
    
    startT800Updates() {
        // Initialize start time
        this.t800StartTime = Date.now();
        
        // Update T-800 HUD elements
        if (this.t800Interval) {
            clearInterval(this.t800Interval);
        }
        
        this.t800Interval = setInterval(() => {
            this.updateT800HUD();
        }, 250); // Update every 250ms (optimized for performance)
    }
    
    stopT800Updates() {
        if (this.t800Interval) {
            clearInterval(this.t800Interval);
            this.t800Interval = null;
        }
    }
    
    updateT800HUD() {
        // Update time display
        const elapsed = Date.now() - this.t800StartTime;
        const hours = Math.floor(elapsed / 3600000);
        const minutes = Math.floor((elapsed % 3600000) / 60000);
        const seconds = Math.floor((elapsed % 60000) / 1000);
        
        const timeStr = `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
        
        const timeEl = document.getElementById('t800Time');
        if (timeEl) {
            timeEl.textContent = timeStr;
        }
        
        // Update memory address (simulate changing hex values)
        const memEl = document.getElementById('t800Mem');
        if (memEl) {
            const memAddr = 0x2000 + Math.floor(Math.random() * 0x1000);
            memEl.textContent = '0x' + memAddr.toString(16).toUpperCase();
        }
        
        // Check if moving
        const isMoving = this.leftJoystick.active || this.rightJoystick.active || 
                        (this.keyStates && Object.values(this.keyStates).some(state => state));
        
        // Update scan status
        const scanEl = document.getElementById('t800Scan');
        if (scanEl) {
            scanEl.textContent = isMoving ? 'TRACKING' : 'ACTIVE';
        }
        
        // Update threat assessment
        const threatEl = document.getElementById('t800Threat');
        if (threatEl) {
            if (isMoving) {
                threatEl.textContent = 'DETECTED';
                threatEl.style.color = '#ffaa00';
            } else {
                threatEl.textContent = 'NONE';
                threatEl.style.color = '#ff0000';
            }
        }
        
        // Update power level (98-100%)
        const powerEl = document.getElementById('t800Power');
        if (powerEl) {
            const power = 98 + Math.floor(Math.random() * 3);
            powerEl.textContent = `${power}%`;
        }
        
        // Update temperature (36-38°C for realistic body temp)
        const tempEl = document.getElementById('t800Temp');
        if (tempEl) {
            const temp = 36 + Math.floor(Math.random() * 3);
            tempEl.textContent = `${temp}°C`;
        }
        
        // Update motor system status
        const motorEl = document.getElementById('t800Motor');
        if (motorEl) {
            motorEl.textContent = isMoving ? 'ENGAGED' : 'NOMINAL';
        }
        
        // Update optical system
        const opticalEl = document.getElementById('t800Optical');
        if (opticalEl) {
            opticalEl.textContent = 'ONLINE';
        }
        
        // Update neural net CPU
        const netEl = document.getElementById('t800Net');
        if (netEl) {
            netEl.textContent = 'ACTIVE';
        }
    }

    async checkVRSupport() {
        if (navigator.xr) {
            try {
                const supported = await navigator.xr.isSessionSupported('immersive-vr');
                if (supported) {
                    this.vrEnabled = true;
                    console.log('VR поддерживается');
                    
                    // Показываем кнопку входа в VR
                    const vrBtn = document.getElementById('vrBtn');
                    if (vrBtn) {
                        vrBtn.classList.remove('hidden');
                        vrBtn.addEventListener('click', () => this.enterVR());
                    }
                } else {
                    console.log('VR не поддерживается браузером');
                }
            } catch (error) {
                console.log('Ошибка проверки VR поддержки:', error);
            }
        } else {
            console.log('WebXR API не доступен');
        }
    }

    async enterVR() {
        if (!this.vrEnabled) {
            alert('VR режим не поддерживается в этом браузере');
            return;
        }

        try {
            console.log('Запуск VR сессии...');
            
            // Запрашиваем VR сессию с необходимыми функциями
            this.xrSession = await navigator.xr.requestSession('immersive-vr', {
                requiredFeatures: ['local-floor'],
                optionalFeatures: ['bounded-floor', 'hand-tracking']
            });

            console.log('VR сессия создана:', this.xrSession);

            // Настройка событий сессии
            this.xrSession.addEventListener('end', () => this.onVRSessionEnded());
            
            // Получаем reference space
            this.xrReferenceSpace = await this.xrSession.requestReferenceSpace('local-floor');
            
            // Настройка контроллеров
            this.setupVRControllers();
            
            // Запуск VR рендер цикла
            this.xrSession.requestAnimationFrame((time, frame) => this.onVRFrame(time, frame));
            
            // Обновляем интерфейс
            this.updateVRUI(true);
            
            console.log('VR режим активирован');
        } catch (error) {
            console.error('Ошибка входа в VR:', error);
            alert('Не удалось войти в VR режим: ' + error.message);
        }
    }

    setupVRControllers() {
        console.log('Настройка VR контроллеров...');
        
        // Слушаем подключение контроллеров
        this.xrSession.addEventListener('inputsourceschange', (event) => {
            console.log('Изменение источников ввода:', event);
            
            if (event.added) {
                event.added.forEach(inputSource => {
                    console.log('Контроллер подключен:', inputSource.handedness, inputSource.targetRayMode);
                    this.controllers.push(inputSource);
                });
            }
            
            if (event.removed) {
                event.removed.forEach(inputSource => {
                    console.log('Контроллер отключен:', inputSource.handedness);
                    const index = this.controllers.indexOf(inputSource);
                    if (index > -1) {
                        this.controllers.splice(index, 1);
                    }
                });
            }
            
            this.updateVRStatus();
        });
    }

    onVRFrame(time, frame) {
        if (!this.xrSession) return;
        
        // Запрашиваем следующий кадр
        this.xrSession.requestAnimationFrame((time, frame) => this.onVRFrame(time, frame));
        
        // Получаем источники ввода
        const inputSources = this.xrSession.inputSources;
        
        // Обрабатываем контроллеры
        this.processVRControllers(frame, inputSources);
    }

    processVRControllers(frame, inputSources) {
        let leftThumbstick = { x: 0, y: 0 };
        let rightThumbstick = { x: 0, y: 0 };
        let triggerPressed = false;
        let gripPressed = false;
        let buttonAPressed = false;
        let buttonBPressed = false;
        
        for (const inputSource of inputSources) {
            if (!inputSource.gamepad) continue;
            
            const gamepad = inputSource.gamepad;
            const hand = inputSource.handedness; // 'left' или 'right'
            
            // Обработка стиков (axes)
            // В WebXR для Oculus Quest: axes[0] и axes[1] - это thumbstick X и Y
            // axes[2] и axes[3] - touchpad (если есть)
            if (gamepad.axes && gamepad.axes.length >= 2) {
                if (hand === 'left') {
                    // Левый стик для поворота/левой стороны
                    leftThumbstick.x = gamepad.axes[0] || 0;
                    leftThumbstick.y = gamepad.axes[1] || 0;
                } else if (hand === 'right') {
                    // Правый стик для движения/правой стороны
                    rightThumbstick.x = gamepad.axes[0] || 0;
                    rightThumbstick.y = gamepad.axes[1] || 0;
                }
            }
            
            // Обработка кнопок
            // В Oculus Quest:
            // buttons[0] - trigger
            // buttons[1] - grip
            // buttons[3] - thumbstick press
            // buttons[4] - button A/X
            // buttons[5] - button B/Y
            if (gamepad.buttons && gamepad.buttons.length > 0) {
                // Trigger (index 0) - сигнал
                if (gamepad.buttons[0] && gamepad.buttons[0].pressed) {
                    triggerPressed = true;
                }
                
                // Grip (index 1) - фонарик
                if (gamepad.buttons[1] && gamepad.buttons[1].pressed) {
                    gripPressed = true;
                }
                
                // Кнопка A/X (index 4) 
                if (gamepad.buttons.length > 4 && gamepad.buttons[4] && gamepad.buttons[4].pressed) {
                    buttonAPressed = true;
                }
                
                // Кнопка B/Y (index 5)
                if (gamepad.buttons.length > 5 && gamepad.buttons[5] && gamepad.buttons[5].pressed) {
                    buttonBPressed = true;
                }
            }
        }
        
        // Применяем движение
        this.updateVRMovement(leftThumbstick, rightThumbstick);
        
        // Обрабатываем кнопки
        this.handleVRButtons(triggerPressed, gripPressed, buttonAPressed, buttonBPressed);
    }

    updateVRMovement(leftStick, rightStick) {
        // Применяем deadzone
        const deadzone = 0.15;
        
        if (Math.abs(leftStick.x) < deadzone) leftStick.x = 0;
        if (Math.abs(leftStick.y) < deadzone) leftStick.y = 0;
        if (Math.abs(rightStick.x) < deadzone) rightStick.x = 0;
        if (Math.abs(rightStick.y) < deadzone) rightStick.y = 0;
        
        let leftSpeed = 0;
        let rightSpeed = 0;
        
        // Дифференциальный режим: правый стик Y = скорость, левый стик X = поворот
        // (согласовано с мобильным управлением: левый = поворот, правый = скорость)
        // Формула соответствует ROS diff_drive_controller и ArduPilot:
        // left_motor = speed + turn (поворот вправо: левый быстрее)
        // right_motor = speed - turn (поворот вправо: правый медленнее)
        const speed = -rightStick.y * this.speedSensitivity;
        const turn = -leftStick.x * this.turnSensitivity;
        
        leftSpeed = speed + turn;
        rightSpeed = speed - turn;
        
        // Ограничение значений
        leftSpeed = Math.max(-100, Math.min(100, leftSpeed));
        rightSpeed = Math.max(-100, Math.min(100, rightSpeed));
        
        // Отправляем команды только если есть изменения
        if (leftSpeed !== this.lastVRLeftSpeed || rightSpeed !== this.lastVRRightSpeed) {
            this.sendMovementCommand(leftSpeed, rightSpeed);
            this.lastVRLeftSpeed = leftSpeed;
            this.lastVRRightSpeed = rightSpeed;
        }
    }

    handleVRButtons(trigger, grip, buttonA, buttonB) {
        // Trigger - сигнал (удерживать)
        if (trigger && !this.vrTriggerPressed) {
            this.startHorn();
            this.vrTriggerPressed = true;
        } else if (!trigger && this.vrTriggerPressed) {
            this.stopHorn();
            this.vrTriggerPressed = false;
        }
        
        // Grip - фонарик (переключение по нажатию)
        if (grip && !this.vrGripPressed) {
            this.toggleFlashlight();
            this.vrGripPressed = true;
        } else if (!grip) {
            this.vrGripPressed = false;
        }
        
        // Кнопка A - смена эффекта
        if (buttonA && !this.vrButtonAPressed) {
            this.cycleEffectMode();
            this.vrButtonAPressed = true;
        } else if (!buttonA) {
            this.vrButtonAPressed = false;
        }
    }

    cycleEffectMode() {
        const modes = ['normal', 'police', 'fire', 'ambulance', 'terminator'];
        const currentIndex = modes.indexOf(this.effectMode);
        const nextIndex = (currentIndex + 1) % modes.length;
        this.effectMode = modes[nextIndex];
        
        this.sendCommand('setEffectMode', { mode: this.effectMode });
        this.updateT800Overlay();
        console.log('Режим эффекта:', this.effectMode);
    }

    updateVRStatus() {
        const vrStatus = document.getElementById('vrStatus');
        if (vrStatus) {
            const controllerCount = this.controllers.length;
            if (controllerCount === 0) {
                vrStatus.textContent = 'Поиск контроллеров...';
            } else {
                const leftController = this.controllers.find(c => c.handedness === 'left');
                const rightController = this.controllers.find(c => c.handedness === 'right');
                
                let status = `Контроллеры: ${controllerCount}\n`;
                if (leftController) status += '✓ Левый ';
                if (rightController) status += '✓ Правый';
                
                vrStatus.textContent = status;
            }
        }
    }

    updateVRUI(inVR) {
        const vrBtn = document.getElementById('vrBtn');
        const vrControls = document.getElementById('vrControls');
        const pcControls = document.getElementById('pcControls');
        const mobileControls = document.getElementById('mobileControls');
        
        if (inVR) {
            if (vrBtn) vrBtn.classList.add('active');
            if (vrControls) vrControls.classList.remove('hidden');
            // Скрыть другие контролы в VR режиме
            if (pcControls) pcControls.classList.add('hidden');
            if (mobileControls) mobileControls.classList.add('hidden');
        } else {
            if (vrBtn) vrBtn.classList.remove('active');
            if (vrControls) vrControls.classList.add('hidden');
            // Восстановить интерфейс
            this.setupInterface();
        }
    }

    onVRSessionEnded() {
        console.log('VR сессия завершена');
        this.xrSession = null;
        this.xrReferenceSpace = null;
        this.controllers = [];
        this.updateVRUI(false);
        
        // Сброс состояния кнопок
        this.vrTriggerPressed = false;
        this.vrGripPressed = false;
        this.vrButtonAPressed = false;
        this.vrButtonBPressed = false;
        this.lastVRLeftSpeed = 0;
        this.lastVRRightSpeed = 0;
        
        // Остановить робота
        this.sendMovementCommand(0, 0);
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
        
        // WiFi настройки
        const saveWiFiBtn = document.getElementById('saveWiFi');
        if (saveWiFiBtn) {
            saveWiFiBtn.addEventListener('click', () => this.saveWiFiConfig());
        }
        
        const restartBtn = document.getElementById('restartDevice');
        if (restartBtn) {
            restartBtn.addEventListener('click', () => this.restartDevice());
        }
        
        // Update handlers
        const checkUpdatesBtn = document.getElementById('checkUpdatesBtn');
        if (checkUpdatesBtn) {
            checkUpdatesBtn.addEventListener('click', () => this.checkForUpdates());
        }
        
        const selectFirmwareBtn = document.getElementById('selectFirmwareBtn');
        if (selectFirmwareBtn) {
            selectFirmwareBtn.addEventListener('click', () => {
                document.getElementById('firmwareFile').click();
            });
        }
        
        const firmwareFile = document.getElementById('firmwareFile');
        if (firmwareFile) {
            firmwareFile.addEventListener('change', (e) => this.onFirmwareSelected(e));
        }
        
        const uploadFirmwareBtn = document.getElementById('uploadFirmwareBtn');
        if (uploadFirmwareBtn) {
            uploadFirmwareBtn.addEventListener('click', () => this.uploadFirmware());
        }
        
        const downloadUpdateBtn = document.getElementById('downloadUpdateBtn');
        if (downloadUpdateBtn) {
            downloadUpdateBtn.addEventListener('click', () => this.downloadAndInstallUpdate());
        }
        
        const autoUpdateCheckbox = document.getElementById('autoUpdate');
        if (autoUpdateCheckbox) {
            autoUpdateCheckbox.addEventListener('change', (e) => this.saveUpdateSettings());
        }
        
        const dontOfferCheckbox = document.getElementById('dontOfferUpdates');
        if (dontOfferCheckbox) {
            dontOfferCheckbox.addEventListener('change', (e) => this.saveUpdateSettings());
        }
    }

    showSettings() {
        const modal = document.getElementById('settingsModal');
        if (modal) {
            modal.classList.remove('hidden');
            
            // Загрузить текущие настройки
            document.getElementById('speedSensitivity').value = this.speedSensitivity;
            document.getElementById('turnSensitivity').value = this.turnSensitivity;
            
            // Установить текущий режим эффектов
            document.querySelector('input[name="effectMode"][value="' + this.effectMode + '"]').checked = true;
            
            // Загрузить статус WiFi
            this.loadWiFiStatus();
            
            // Загрузить информацию о версии и настройках обновлений
            this.loadUpdateInfo();
        }
    }

    saveSettings() {
        // Сохранить настройки
        this.speedSensitivity = parseInt(document.getElementById('speedSensitivity').value);
        this.turnSensitivity = parseInt(document.getElementById('turnSensitivity').value);
        
        const effectMode = document.querySelector('input[name="effectMode"]:checked');
        
        if (effectMode) {
            this.effectMode = effectMode.value;
            this.sendCommand('setEffectMode', { mode: this.effectMode });
            this.updateT800Overlay();
        }
        
        // Сохранить в localStorage
        localStorage.setItem('microbox-settings', JSON.stringify({
            speedSensitivity: this.speedSensitivity,
            turnSensitivity: this.turnSensitivity,
            effectMode: this.effectMode
        }));
        
        // Закрыть модальное окно
        document.getElementById('settingsModal').classList.add('hidden');
        
        console.log('Настройки сохранены');
    }

    // Update functions
    async loadUpdateInfo() {
        try {
            // Load current version
            const versionResponse = await fetch('/api/update/current');
            if (versionResponse.ok) {
                const versionData = await versionResponse.json();
                document.getElementById('currentVersion').textContent = versionData.version;
                const releaseNameEl = document.getElementById('releaseName');
                if (releaseNameEl && versionData.releaseName) {
                    releaseNameEl.textContent = `Релиз: ${versionData.releaseName}`;
                }
            }
            
            // Load update settings
            const settingsResponse = await fetch('/api/update/settings');
            if (settingsResponse.ok) {
                const settings = await settingsResponse.json();
                document.getElementById('autoUpdate').checked = settings.autoUpdate || false;
                document.getElementById('dontOfferUpdates').checked = settings.dontOffer || false;
            }
        } catch (error) {
            console.error('Error loading update info:', error);
        }
    }
    
    async checkForUpdates() {
        const btn = document.getElementById('checkUpdatesBtn');
        btn.disabled = true;
        btn.textContent = 'Проверка...';
        
        try {
            // Получаем текущую версию с устройства
            const currentVersionResponse = await fetch('/api/update/current');
            if (!currentVersionResponse.ok) {
                throw new Error('Не удалось получить текущую версию');
            }
            const currentVersionData = await currentVersionResponse.json();
            const currentVersion = currentVersionData.version;
            
            if (!currentVersion) {
                throw new Error('Не удалось получить текущую версию устройства');
            }
            
            // Проверяем обновления на GitHub API напрямую с клиента
            const githubApiUrl = `https://api.github.com/repos/${this.GITHUB_REPO}/releases/latest`;
            const githubResponse = await fetch(githubApiUrl, {
                headers: {
                    'Accept': 'application/vnd.github+json',  // Current stable GitHub API version
                    'User-Agent': 'MicroBox-Web-Client'
                }
            });
            
            if (!githubResponse.ok) {
                throw new Error('Не удалось получить информацию о релизах с GitHub');
            }
            
            const releaseData = await githubResponse.json();
            
            // Валидация обязательных полей GitHub API response
            if (!releaseData || !releaseData.tag_name) {
                throw new Error('Некорректный ответ от GitHub API');
            }
            
            // Извлекаем информацию о релизе
            const latestVersion = releaseData.tag_name;
            const releaseName = releaseData.name || latestVersion;
            const releaseNotes = releaseData.body || 'Нет описания';
            const publishedAt = releaseData.published_at;
            
            // Находим .bin файл для загрузки
            let downloadUrl = '';
            if (releaseData.assets && Array.isArray(releaseData.assets) && releaseData.assets.length > 0) {
                const binAsset = releaseData.assets.find(asset => 
                    asset && asset.name && asset.name.endsWith('-release.bin') && asset.browser_download_url
                );
                if (binAsset && binAsset.browser_download_url) {
                    downloadUrl = binAsset.browser_download_url;
                }
            }
            
            // Сравниваем версии
            const hasUpdate = this.isVersionNewer(currentVersion, latestVersion);
            
            if (hasUpdate) {
                // Show update available section
                const updateSection = document.getElementById('updateAvailable');
                updateSection.classList.remove('hidden');
                document.getElementById('newVersion').textContent = latestVersion;
                document.getElementById('newReleaseName').textContent = `Релиз: ${releaseName}`;
                document.getElementById('releaseNotes').textContent = releaseNotes;
                
                // Store download URL for later
                this.updateDownloadUrl = downloadUrl;
                
                console.log('Доступно обновление:', {
                    current: currentVersion,
                    latest: latestVersion,
                    downloadUrl: downloadUrl
                });
            } else {
                alert('У вас установлена последняя версия прошивки!');
            }
        } catch (error) {
            console.error('Error checking updates:', error);
            alert('Ошибка при проверке обновлений: ' + error.message);
        } finally {
            btn.disabled = false;
            btn.textContent = 'Проверить обновления';
        }
    }
    
    // Функция сравнения версий
    isVersionNewer(currentVersion, latestVersion) {
        // Убираем префикс 'v' если есть и берем только числовую часть до дефиса
        const cleanCurrent = currentVersion.replace(/^v/, '').split('-')[0];
        const cleanLatest = latestVersion.replace(/^v/, '').split('-')[0];
        
        const currentParts = cleanCurrent.split('.').map(part => {
            const num = parseInt(part, 10);
            return isNaN(num) ? 0 : num;
        });
        const latestParts = cleanLatest.split('.').map(part => {
            const num = parseInt(part, 10);
            return isNaN(num) ? 0 : num;
        });
        
        for (let i = 0; i < Math.max(currentParts.length, latestParts.length); i++) {
            const current = currentParts[i] || 0;
            const latest = latestParts[i] || 0;
            
            if (latest > current) return true;
            if (latest < current) return false;
        }
        
        return false;
    }
    
    async downloadAndInstallUpdate() {
        if (!this.updateDownloadUrl) {
            alert('URL обновления не найден');
            return;
        }
        
        if (!confirm('Начать загрузку и установку обновления? Это займет несколько минут.')) {
            return;
        }
        
        alert('Функция автоматической загрузки еще в разработке. Пожалуйста, скачайте файл вручную с GitHub и загрузите через форму ниже.');
    }
    
    onFirmwareSelected(event) {
        const file = event.target.files[0];
        if (file) {
            document.getElementById('selectedFileName').textContent = file.name;
            document.getElementById('uploadFirmwareBtn').disabled = false;
        }
    }
    
    async uploadFirmware() {
        const fileInput = document.getElementById('firmwareFile');
        const file = fileInput.files[0];
        
        if (!file) {
            alert('Выберите файл прошивки');
            return;
        }
        
        if (!file.name.endsWith('.bin')) {
            alert('Файл должен иметь расширение .bin');
            return;
        }
        
        if (!confirm('Вы уверены, что хотите обновить прошивку? Робот будет перезагружен.')) {
            return;
        }
        
        const progressDiv = document.getElementById('uploadProgress');
        const progressFill = document.getElementById('progressFill');
        const progressText = document.getElementById('progressText');
        const uploadBtn = document.getElementById('uploadFirmwareBtn');
        
        progressDiv.classList.remove('hidden');
        uploadBtn.disabled = true;
        uploadBtn.textContent = 'Загрузка...';
        
        try {
            const formData = new FormData();
            formData.append('update', file);
            
            const xhr = new XMLHttpRequest();
            
            xhr.upload.addEventListener('progress', (e) => {
                if (e.lengthComputable) {
                    const percentComplete = (e.loaded / e.total) * 100;
                    progressFill.style.width = percentComplete + '%';
                    progressText.textContent = Math.round(percentComplete) + '%';
                }
            });
            
            xhr.addEventListener('load', () => {
                if (xhr.status === 200) {
                    alert('Обновление завершено! Устройство будет перезагружено.');
                    progressFill.style.width = '100%';
                    progressText.textContent = '100%';
                    
                    // Wait for reboot
                    setTimeout(() => {
                        window.location.reload();
                    }, 5000);
                } else {
                    alert('Ошибка загрузки прошивки: ' + xhr.statusText);
                    uploadBtn.disabled = false;
                    uploadBtn.textContent = 'Загрузить прошивку';
                }
            });
            
            xhr.addEventListener('error', () => {
                alert('Ошибка при загрузке файла');
                uploadBtn.disabled = false;
                uploadBtn.textContent = 'Загрузить прошивку';
            });
            
            xhr.open('POST', '/api/update/upload');
            xhr.send(formData);
            
        } catch (error) {
            console.error('Error uploading firmware:', error);
            alert('Ошибка при загрузке прошивки');
            uploadBtn.disabled = false;
            uploadBtn.textContent = 'Загрузить прошивку';
        }
    }
    
    async saveUpdateSettings() {
        const autoUpdate = document.getElementById('autoUpdate').checked;
        const dontOffer = document.getElementById('dontOfferUpdates').checked;
        
        try {
            const response = await fetch('/api/update/settings', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ autoUpdate, dontOffer })
            });
            
            if (response.ok) {
                console.log('Update settings saved');
            }
        } catch (error) {
            console.error('Error saving update settings:', error);
        }
    }

    loadSettings() {
        const saved = localStorage.getItem('microbox-settings');
        if (saved) {
            try {
                const settings = JSON.parse(saved);
                this.speedSensitivity = settings.speedSensitivity || 80;
                this.turnSensitivity = settings.turnSensitivity || 70;
                this.controlMode = 'differential';
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
        let lastPingTime = 0;
        
        const loop = () => {
            // Обработка геймпада
            this.processGamepad();
            
            // Проверка соединения - каждые 5 секунд
            const now = Date.now();
            if (now - lastPingTime >= 5000) {
                this.sendCommand('ping');
                lastPingTime = now;
            }
            
            requestAnimationFrame(loop);
        };
        
        loop();
    }

    async loadWiFiStatus() {
        try {
            const response = await fetch('/api/wifi/status');
            const data = await response.json();
            
            const statusDiv = document.getElementById('wifiStatus');
            if (statusDiv) {
                let statusHTML = '<strong>Текущий статус:</strong><br>';
                statusHTML += `Режим: ${data.mode}<br>`;
                statusHTML += `Подключено: ${data.connected ? 'Да' : 'Нет'}<br>`;
                statusHTML += `IP адрес: ${data.ip}<br>`;
                statusHTML += `Имя устройства: ${data.deviceName}<br>`;
                if (data.savedSSID) {
                    statusHTML += `Сохраненная сеть: ${data.savedSSID}`;
                }
                statusDiv.innerHTML = statusHTML;
            }
            
            // Заполнить поля формы
            if (data.savedSSID) {
                document.getElementById('wifiSSID').value = data.savedSSID;
            }
            if (data.savedMode) {
                document.getElementById('wifiMode').value = data.savedMode;
            }
        } catch (error) {
            console.error('Ошибка загрузки статуса WiFi:', error);
        }
    }

    async saveWiFiConfig() {
        const ssid = document.getElementById('wifiSSID').value;
        const password = document.getElementById('wifiPassword').value;
        const mode = document.getElementById('wifiMode').value;
        
        if (!ssid) {
            alert('Пожалуйста, введите SSID сети');
            return;
        }
        
        if (ssid.length > 32) {
            alert('SSID не может быть длиннее 32 символов');
            return;
        }
        
        if (password && password.length < 8) {
            alert('Пароль должен быть минимум 8 символов');
            return;
        }
        
        try {
            const response = await fetch('/api/wifi/config', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    ssid: ssid,
                    password: password,
                    mode: mode
                })
            });
            
            const data = await response.json();
            
            if (data.status === 'ok') {
                alert('Настройки WiFi сохранены! Перезагрузите устройство для применения.');
            } else {
                alert('Ошибка: ' + data.message);
            }
        } catch (error) {
            console.error('Ошибка сохранения WiFi:', error);
            alert('Ошибка сохранения настроек');
        }
    }

    async restartDevice() {
        if (!confirm('Вы уверены, что хотите перезагрузить устройство?')) {
            return;
        }
        
        try {
            // Отправляем подтверждение для безопасности
            const formData = new FormData();
            formData.append('confirm', 'yes');
            
            await fetch('/api/restart', {
                method: 'POST',
                body: formData
            });
            
            alert('Устройство перезагружается... Подождите около 30 секунд.');
            
            // Попытаться переподключиться через 30 секунд
            setTimeout(() => {
                window.location.reload();
            }, 30000);
        } catch (error) {
            console.error('Ошибка перезагрузки:', error);
        }
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