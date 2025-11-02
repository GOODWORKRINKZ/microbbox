// МикроББокс - Система управления

// Расширенный логгер с поддержкой консоли, API и страницы
const Logger = {
    LEVELS: { ERROR: 0, WARN: 1, INFO: 2, DEBUG: 3 },
    currentLevel: 2, // INFO по умолчанию (ERROR=0, WARN=1, INFO=2, DEBUG=3)
    
    // Настройки вывода
    outputs: {
        console: true,      // Логировать в консоль браузера
        api: false,         // Логировать в API (Serial Monitor)
        page: false,        // Логировать на страницу (для VR)
        pageElementId: null // ID элемента для вывода на страницу
    },
    
    // Буфер логов для страницы
    pageBuffer: [],
    maxPageBuffer: 100,
    
    error: function(...args) {
        if (this.currentLevel >= this.LEVELS.ERROR) {
            this._log('ERROR', ...args);
        }
    },
    warn: function(...args) {
        if (this.currentLevel >= this.LEVELS.WARN) {
            this._log('WARN', ...args);
        }
    },
    info: function(...args) {
        if (this.currentLevel >= this.LEVELS.INFO) {
            this._log('INFO', ...args);
        }
    },
    debug: function(...args) {
        if (this.currentLevel >= this.LEVELS.DEBUG) {
            this._log('DEBUG', ...args);
        }
    },
    
    // Специальный метод для VR логов
    vr: function(...args) {
        this._log('VR', ...args);
    },
    
    // Внутренний метод логирования
    _log: function(level, ...args) {
        const timestamp = new Date().toISOString().substring(11, 23); // HH:MM:SS.mmm
        const message = args.map(arg => 
            typeof arg === 'object' ? JSON.stringify(arg) : String(arg)
        ).join(' ');
        
        const formattedMessage = `[${timestamp}] [${level}] ${message}`;
        
        // Вывод в консоль
        if (this.outputs.console) {
            switch(level) {
                case 'ERROR': console.error(formattedMessage); break;
                case 'WARN': console.warn(formattedMessage); break;
                default: console.log(formattedMessage); break;
            }
        }
        
        // Вывод на страницу
        if (this.outputs.page && this.outputs.pageElementId) {
            this._logToPage(formattedMessage);
        }
        
        // Вывод в API (Serial Monitor) - только для важных сообщений
        if (this.outputs.api && (level === 'ERROR' || level === 'WARN' || level === 'VR')) {
            this._logToAPI(level, message);
        }
    },
    
    // Логирование на страницу
    _logToPage: function(message) {
        this.pageBuffer.push(message);
        if (this.pageBuffer.length > this.maxPageBuffer) {
            this.pageBuffer.shift();
        }
        
        const element = document.getElementById(this.outputs.pageElementId);
        if (element) {
            element.textContent = this.pageBuffer.join('\n');
            // Автоскролл вниз
            element.scrollTop = element.scrollHeight;
        }
    },
    
    // Логирование в API (Serial Monitor)
    _logToAPI: async function(level, message) {
        try {
            const response = await fetch('/api/vr-log', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    level: level,
                    message: message,
                    timestamp: new Date().toISOString()
                })
            });
            
            if (!response.ok) {
                throw new Error(`API returned ${response.status}: ${response.statusText}`);
            }
        } catch (error) {
            // Если упало логирование в API - просто логируем в консоль и идём дальше
            // Не создаём бесконечную рекурсию, просто console.error напрямую
            console.error('[Logger] Failed to send to API:', error.message, '- continuing...');
            // Продолжаем работу, не бросаем исключение
        }
    },
    
    setLevel: function(level) {
        this.currentLevel = level;
    },
    
    // Включить/выключить вывод в разные места
    enableConsole: function(enable = true) {
        this.outputs.console = enable;
    },
    
    enableAPI: function(enable = true) {
        this.outputs.api = enable;
    },
    
    enablePage: function(elementId, enable = true) {
        this.outputs.page = enable;
        this.outputs.pageElementId = elementId;
    },
    
    // Очистить буфер страницы
    clearPageBuffer: function() {
        this.pageBuffer = [];
        const element = document.getElementById(this.outputs.pageElementId);
        if (element) {
            element.textContent = '';
        }
    },
    
    // Получить все логи как текст
    getPageLogs: function() {
        return this.pageBuffer.join('\n');
    }
};

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
        
        // T-800 overlay state
        this.t800Interval = null;
        this.t800StartTime = null;
        
        // Help animation
        this.helpAnimationId = null;
        
        // Command Controller - централизованная система отправки команд
        this.commandController = {
            targetThrottle: 1500,    // Целевое положение газа (PWM)
            targetSteering: 1500,    // Целевое положение руля (PWM)
            lastSentThrottle: 1500,  // Последнее отправленное
            lastSentSteering: 1500,  // Последнее отправленное
            lastSendTime: 0,         // Время последней отправки
            sendInterval: 250,       // Интервал отправки (мс) - получим с сервера
            commandTimeout: 500,     // Таймаут на сервере (мс) - получим с сервера
            isSending: false,        // Флаг: выполняется ли отправка прямо сейчас
            fetchTimeout: 250        // Таймаут HTTP запроса (мс)
        };
        
        this.init();
    }

    async init() {
        console.log('Инициализация МикроББокс контроллера...');
        
        // Получаем конфигурацию с сервера
        await this.loadServerConfig();
        
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
        
        // Проверка версии после обновления
        setTimeout(() => {
            this.checkVersionAfterUpdate();
        }, 3000);
        
        // Check for updates on startup (after a delay)
        setTimeout(() => {
            this.checkForUpdatesOnStartup();
        }, 5000);
        
        console.log('Инициализация завершена');
    }
    
    async loadServerConfig() {
        try {
            Logger.info('Загрузка конфигурации с сервера...');
            const response = await fetch('/api/config');
            if (!response.ok) {
                Logger.warn('Не удалось загрузить конфиг, используем значения по умолчанию');
                return;
            }
            
            const config = await response.json();
            
            // Обновляем параметры CommandController
            if (config.motorCommandTimeout) {
                this.commandController.commandTimeout = config.motorCommandTimeout;
            }
            
            // Интервал отправки = 60% от таймаута (для запаса)
            this.commandController.sendInterval = Math.floor(this.commandController.commandTimeout * 0.6);
            
            Logger.info(`Конфигурация загружена: timeout=${this.commandController.commandTimeout}ms, interval=${this.commandController.sendInterval}ms`);
        } catch (error) {
            Logger.error('Ошибка загрузки конфигурации:', error);
        }
    }
    
    async checkVersionAfterUpdate() {
        try {
            // Получаем текущую версию с устройства
            const response = await fetch('/api/update/current');
            if (!response.ok) return;
            
            const data = await response.json();
            const currentVersion = data.version;
            
            if (!currentVersion) return;
            
            // Получаем сохраненную версию из localStorage
            const savedVersion = localStorage.getItem('microbbox_version');
            
            // Если версия изменилась (но не первый запуск)
            if (savedVersion && savedVersion !== currentVersion) {
                Logger.info('Обновление обнаружено:', savedVersion, '->', currentVersion);
                this.showUpdateSuccessNotification(savedVersion, currentVersion);
                localStorage.setItem('microbbox_version', currentVersion);
            } else if (!savedVersion) {
                Logger.info('Первый запуск, сохраняем версию:', currentVersion);
                localStorage.setItem('microbbox_version', currentVersion);
            }
        } catch (error) {
            Logger.error('Не удалось проверить версию после обновления:', error);
        }
    }
    
    showUpdateSuccessNotification(oldVersion, newVersion) {
        Logger.info('Показ уведомления об обновлении:', oldVersion, '->', newVersion);
        // Создаем красивое уведомление о успешном обновлении
        const notification = document.createElement('div');
        notification.className = 'update-success-notification';
        notification.innerHTML = `
            <div class="notification-content">
                <div class="notification-icon">✓</div>
                <div class="notification-text">
                    <h3>Обновление успешно!</h3>
                    <p>Прошивка обновлена: <strong>${oldVersion}</strong> → <strong>${newVersion}</strong></p>
                </div>
                <button class="notification-close" onclick="this.parentElement.parentElement.remove()">×</button>
            </div>
        `;
        
        document.body.appendChild(notification);
        
        // Автоматически скрыть через 10 секунд
        setTimeout(() => {
            if (notification.parentElement) {
                notification.classList.add('fade-out');
                setTimeout(() => notification.remove(), 500);
            }
        }, 10000);
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

        // Help button
        const helpBtn = document.getElementById('helpBtn') || document.getElementById('mobileHelp');
        if (helpBtn) {
            helpBtn.addEventListener('click', () => this.showHelp());
        }

        // Настройки для ПК
        const pcSettingsBtn = document.getElementById('pcSettingsBtn');
        if (pcSettingsBtn) {
            pcSettingsBtn.addEventListener('click', () => this.showSettings());
        }

        // Настройки для мобильных
        const settingsBtn = document.getElementById('mobileSettings');
        if (settingsBtn) {
            settingsBtn.addEventListener('click', () => this.showSettings());
        }

        // VR Debug кнопка
        const vrDebugBtn = document.getElementById('vrDebugBtn');
        if (vrDebugBtn) {
            vrDebugBtn.addEventListener('click', () => this.sendVRDebugLog());
        }
        
        // VR Settings кнопка
        const vrSettingsBtn = document.getElementById('vrSettingsBtn');
        if (vrSettingsBtn) {
            vrSettingsBtn.addEventListener('click', () => this.showSettings());
        }
        
        // VR Debug закрыть
        const vrDebugClose = document.getElementById('vrDebugClose');
        if (vrDebugClose) {
            vrDebugClose.addEventListener('click', () => this.hideVRDebugPanel());
        }
        
        // VR Live Log кнопка
        const vrLiveLogBtn = document.getElementById('vrLiveLogBtn');
        if (vrLiveLogBtn) {
            vrLiveLogBtn.addEventListener('click', () => this.toggleVRLiveLog());
        }
        
        // VR Live Log закрыть
        const vrLiveLogClose = document.getElementById('vrLiveLogClose');
        if (vrLiveLogClose) {
            vrLiveLogClose.addEventListener('click', () => this.hideVRLiveLog());
        }
        
        // VR Log очистить
        const vrLogClear = document.getElementById('vrLogClear');
        if (vrLogClear) {
            vrLogClear.addEventListener('click', () => Logger.clearPageBuffer());
        }
        
        // VR Log to Serial checkbox
        const vrLogToSerial = document.getElementById('vrLogToSerial');
        if (vrLogToSerial) {
            vrLogToSerial.addEventListener('change', (e) => {
                Logger.enableAPI(e.target.checked);
                Logger.vr('Live logging to Serial Monitor:', e.target.checked ? 'ENABLED' : 'DISABLED');
            });
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
        let touchId = null; // Track which touch is controlling this joystick
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

        const handleTouchStart = (e) => {
            // Only handle if not already dragging
            if (isDragging) return;
            
            // Get the first touch that started on this element
            const touch = e.changedTouches[0];
            touchId = touch.identifier;
            
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
            
            const clientX = e.clientX;
            const clientY = e.clientY;
            
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

            knob.style.transform = 'translate(calc(-50% + ' + x + 'px), calc(-50% + ' + y + 'px))';

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

        const handleTouchMove = (e) => {
            if (!isDragging || touchId === null) return;
            
            // Find the touch that belongs to this joystick
            let touch = null;
            for (let i = 0; i < e.touches.length; i++) {
                if (e.touches[i].identifier === touchId) {
                    touch = e.touches[i];
                    break;
                }
            }
            
            if (!touch) return;
            
            const clientX = touch.clientX;
            const clientY = touch.clientY;
            
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

            knob.style.transform = 'translate(calc(-50% + ' + x + 'px), calc(-50% + ' + y + 'px))';

            // Нормализация значений (-1 до 1)
            const normalizedX = x / maxDistance;
            const normalizedY = -y / maxDistance; // Инвертируем Y
            
            if (side === 'left') {
                this.leftJoystick = { x: normalizedX, y: 0, active: true };
            } else {
                this.rightJoystick = { x: 0, y: normalizedY, active: true };
            }
            
            this.updateMovement();
            
            e.preventDefault();
        };

        const handleEnd = () => {
            if (!isDragging) return;
            
            isDragging = false;
            element.classList.remove('active');
            knob.style.transform = 'translate(-50%, -50%)';
            
            if (side === 'left') {
                this.leftJoystick = { x: 0, y: 0, active: false };
            } else {
                this.rightJoystick = { x: 0, y: 0, active: false };
            }
            
            this.updateMovement();
        };

        const handleTouchEnd = (e) => {
            if (!isDragging || touchId === null) return;
            
            // Check if our touch ended
            let touchEnded = false;
            for (let i = 0; i < e.changedTouches.length; i++) {
                if (e.changedTouches[i].identifier === touchId) {
                    touchEnded = true;
                    break;
                }
            }
            
            if (!touchEnded) return;
            
            isDragging = false;
            touchId = null;
            element.classList.remove('active');
            knob.style.transform = 'translate(-50%, -50%)';
            
            if (side === 'left') {
                this.leftJoystick = { x: 0, y: 0, active: false };
            } else {
                this.rightJoystick = { x: 0, y: 0, active: false };
            }
            
            this.updateMovement();
            
            e.preventDefault();
        };

        // Мышь события
        element.addEventListener('mousedown', handleStart);
        document.addEventListener('mousemove', handleMove);
        document.addEventListener('mouseup', handleEnd);

        // Сенсорные события - используем специфичные обработчики для multi-touch
        element.addEventListener('touchstart', handleTouchStart, { passive: false });
        element.addEventListener('touchmove', handleTouchMove, { passive: false });
        element.addEventListener('touchend', handleTouchEnd, { passive: false });
        element.addEventListener('touchcancel', handleTouchEnd, { passive: false });
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
        let throttleNorm = 0; // -1..+1
        let steeringNorm = 0; // -1..+1
        
        // WASD или стрелки
        const forward = this.keyStates['KeyW'] || this.keyStates['ArrowUp'];
        const backward = this.keyStates['KeyS'] || this.keyStates['ArrowDown'];
        const left = this.keyStates['KeyA'] || this.keyStates['ArrowLeft'];
        const right = this.keyStates['KeyD'] || this.keyStates['ArrowRight'];
        
        if (forward) throttleNorm = 1.0;
        if (backward) throttleNorm = -1.0;
        if (left) steeringNorm = -1.0;
        if (right) steeringNorm = 1.0;
        
        // Простое преобразование в PWM и обновление командного контроллера
        const throttle = Math.round(1500 + (throttleNorm * 500 * this.speedSensitivity / 100));
        const steering = Math.round(1500 + (steeringNorm * 500 * this.turnSensitivity / 100));
        
        this.commandController.targetThrottle = Math.max(1000, Math.min(2000, throttle));
        this.commandController.targetSteering = Math.max(1000, Math.min(2000, steering));
    }

    handleControlButton(e, pressed) {
        const direction = e.target.dataset.direction;
        
        if (pressed) {
            e.target.classList.add('active');
            
            // Преобразуем в PWM (1000-2000, центр 1500)
            const forwardPWM = Math.round(1500 + (500 * this.speedSensitivity / 100));
            const backwardPWM = Math.round(1500 - (500 * this.speedSensitivity / 100));
            const leftSteer = Math.round(1500 - (500 * this.turnSensitivity / 100));
            const rightSteer = Math.round(1500 + (500 * this.turnSensitivity / 100));
            
            switch (direction) {
                case 'forward':
                    this.commandController.targetThrottle = forwardPWM;
                    this.commandController.targetSteering = 1500;
                    break;
                case 'backward':
                    this.commandController.targetThrottle = backwardPWM;
                    this.commandController.targetSteering = 1500;
                    break;
                case 'left':
                    this.commandController.targetThrottle = 1500;
                    this.commandController.targetSteering = leftSteer;
                    break;
                case 'right':
                    this.commandController.targetThrottle = 1500;
                    this.commandController.targetSteering = rightSteer;
                    break;
                case 'stop':
                    this.commandController.targetThrottle = 1500;
                    this.commandController.targetSteering = 1500;
                    break;
            }
        } else {
            e.target.classList.remove('active');
            if (direction !== 'stop') {
                this.commandController.targetThrottle = 1500;
                this.commandController.targetSteering = 1500;
            }
        }
    }

    updateMovement() {
        // Простое преобразование стиков в PWM (1000-2000, центр 1500)
        const throttle = Math.round(1500 + (this.rightJoystick.y * 500 * this.speedSensitivity / 100));
        const steering = Math.round(1500 + (this.leftJoystick.x * 500 * this.turnSensitivity / 100));
        
        // Ограничение диапазона
        this.commandController.targetThrottle = Math.max(1000, Math.min(2000, throttle));
        this.commandController.targetSteering = Math.max(1000, Math.min(2000, steering));
        
        // Команда будет отправлена автоматически из главного цикла
    }

    // БЫСТРЫЙ метод отправки команд движения через GET (оптимизация)
    async sendMoveCommand(throttle, steering) {
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), this.commandController.fetchTimeout);
        
        try {
            // GET запрос намного быстрее - нет парсинга JSON на сервере
            const response = await fetch(`/move?t=${throttle}&s=${steering}`, {
                method: 'GET',
                signal: controller.signal
            });
            
            clearTimeout(timeoutId);
            
            if (response.ok) {
                this.updateConnectionStatus(true);
                return true;
            } else {
                this.updateConnectionStatus(false);
                console.error('Ошибка команды движения:', response.statusText);
                return false;
            }
        } catch (error) {
            clearTimeout(timeoutId);
            
            if (error.name === 'AbortError') {
                console.warn(`Команда движения превысила таймаут ${this.commandController.fetchTimeout}мс`);
            } else {
                console.error('Ошибка соединения:', error);
            }
            
            this.updateConnectionStatus(false);
            return false;
        }
    }

    async sendCommand(command, data = {}) {
        // Создаём AbortController для таймаута
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), this.commandController.fetchTimeout);
        
        try {
            const response = await fetch('/command', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    command: command,
                    ...data
                }),
                signal: controller.signal
            });
            
            clearTimeout(timeoutId);
            
            if (response.ok) {
                this.updateConnectionStatus(true);
                return await response.json();
            } else {
                this.updateConnectionStatus(false);
                console.error('Ошибка команды:', response.statusText);
            }
        } catch (error) {
            clearTimeout(timeoutId);
            
            if (error.name === 'AbortError') {
                console.warn(`Команда превысила таймаут ${this.commandController.fetchTimeout}мс`);
            } else {
                console.error('Ошибка соединения:', error);
            }
            
            this.updateConnectionStatus(false);
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
        Logger.info('🔍 Проверка поддержки VR...');
        Logger.debug('User Agent:', navigator.userAgent);
        Logger.debug('Platform:', navigator.platform);
        
        if (navigator.xr) {
            Logger.info('✓ WebXR API доступен');
            
            try {
                // Проверяем поддержку immersive-vr
                const supported = await navigator.xr.isSessionSupported('immersive-vr');
                Logger.debug('immersive-vr supported:', supported);
                
                if (supported) {
                    this.vrEnabled = true;
                    Logger.info('✓ VR режим поддерживается!');
                    Logger.info('🥽 Кнопка VR будет показана');
                    
                    // Показываем кнопку входа в VR
                    const vrBtn = document.getElementById('vrBtn');
                    if (vrBtn) {
                        vrBtn.classList.remove('hidden');
                        vrBtn.addEventListener('click', () => this.enterVR());
                        Logger.debug('✓ Кнопка VR активирована');
                    } else {
                        Logger.warn('✗ Элемент vrBtn не найден в DOM');
                    }
                } else {
                    Logger.warn('✗ VR не поддерживается этим браузером');
                    Logger.info('💡 Используйте Oculus Browser на Quest гарнитуре');
                }
            } catch (error) {
                Logger.error('✗ Ошибка проверки VR поддержки:', error.message);
                Logger.debug('Error details:', error);
            }
        } else {
            Logger.warn('✗ WebXR API не доступен');
            Logger.info('💡 WebXR требуется для VR режима');
            Logger.info('💡 Используйте современный браузер с поддержкой WebXR');
            Logger.info('💡 Рекомендуется: Oculus Browser на Quest гарнитуре');
        }
    }

    async enterVR() {
        if (!this.vrEnabled) {
            alert('VR режим не поддерживается в этом браузере');
            return;
        }

        try {
            Logger.vr('🥽 Запуск VR сессии...');
            
            // Запрашиваем VR сессию с необходимыми функциями
            this.xrSession = await navigator.xr.requestSession('immersive-vr', {
                requiredFeatures: ['local-floor'],
                optionalFeatures: ['bounded-floor', 'hand-tracking']
            });

            Logger.vr('✓ VR сессия создана успешно');

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
            
            Logger.vr('✓ VR режим полностью активирован');
        } catch (error) {
            Logger.error('Ошибка входа в VR:', error.message);
            alert('Не удалось войти в VR режим: ' + error.message);
        }
    }

    setupVRControllers() {
        Logger.vr('🎮 Настройка VR контроллеров...');
        
        // Слушаем подключение контроллеров
        this.xrSession.addEventListener('inputsourceschange', (event) => {
            Logger.debug('Изменение источников ввода VR');
            
            if (event.added) {
                event.added.forEach(inputSource => {
                    Logger.vr(`✓ Контроллер подключен: ${inputSource.handedness} (${inputSource.targetRayMode})`);
                    this.controllers.push(inputSource);
                });
            }
            
            if (event.removed) {
                event.removed.forEach(inputSource => {
                    Logger.vr(`✗ Контроллер отключен: ${inputSource.handedness}`);
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
        
        // Простое преобразование стиков VR в PWM и обновление командного контроллера
        const throttle = Math.round(1500 + (-rightStick.y * 500 * this.speedSensitivity / 100));
        const steering = Math.round(1500 + (-leftStick.x * 500 * this.turnSensitivity / 100));
        
        this.commandController.targetThrottle = Math.max(1000, Math.min(2000, throttle));
        this.commandController.targetSteering = Math.max(1000, Math.min(2000, steering));
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
        
        // Остановить робота (сброс командного контроллера)
        this.commandController.targetThrottle = 1500;
        this.commandController.targetSteering = 1500;
    }

    // Сбор VR диагностической информации
    async collectVRDebugInfo() {
        const debugInfo = {
            timestamp: new Date().toISOString(),
            browser: this.getBrowserName(),
            userAgent: navigator.userAgent,
            platform: navigator.platform,
            language: navigator.language,
            screenWidth: window.screen.width,
            screenHeight: window.screen.height,
            devicePixelRatio: window.devicePixelRatio,
            
            // WebXR информация
            xrSupported: !!navigator.xr,
            vrSessionActive: !!this.xrSession,
            
            // Информация о контроллерах
            controllersCount: this.controllers.length,
            controllers: []
        };
        
        // Проверка поддержки различных VR сессий
        if (navigator.xr) {
            try {
                debugInfo.immersiveVrSupported = await navigator.xr.isSessionSupported('immersive-vr');
                debugInfo.immersiveArSupported = await navigator.xr.isSessionSupported('immersive-ar');
                debugInfo.inlineSupported = await navigator.xr.isSessionSupported('inline');
            } catch (error) {
                debugInfo.sessionCheckError = error.message;
            }
        }
        
        // Детальная информация о контроллерах
        if (this.xrSession && this.xrSession.inputSources) {
            for (const inputSource of this.xrSession.inputSources) {
                const controllerInfo = {
                    handedness: inputSource.handedness,
                    targetRayMode: inputSource.targetRayMode,
                    profiles: inputSource.profiles,
                    hasGamepad: !!inputSource.gamepad
                };
                
                if (inputSource.gamepad) {
                    controllerInfo.gamepad = {
                        id: inputSource.gamepad.id,
                        axesCount: inputSource.gamepad.axes.length,
                        buttonsCount: inputSource.gamepad.buttons.length,
                        axes: Array.from(inputSource.gamepad.axes),
                        buttons: inputSource.gamepad.buttons.map(btn => ({
                            pressed: btn.pressed,
                            touched: btn.touched,
                            value: btn.value
                        }))
                    };
                }
                
                debugInfo.controllers.push(controllerInfo);
            }
        }
        
        // Информация о VR сессии
        if (this.xrSession) {
            debugInfo.vrSession = {
                environmentBlendMode: this.xrSession.environmentBlendMode,
                interactionMode: this.xrSession.interactionMode,
                visibilityState: this.xrSession.visibilityState,
                renderState: {
                    depthNear: this.xrSession.renderState.depthNear,
                    depthFar: this.xrSession.renderState.depthFar
                }
            };
        }
        
        // Состояние приложения
        debugInfo.app = {
            deviceType: this.deviceType,
            controlMode: this.controlMode,
            effectMode: this.effectMode,
            isConnected: this.isConnected,
            vrEnabled: this.vrEnabled,
            speedSensitivity: this.speedSensitivity,
            turnSensitivity: this.turnSensitivity
        };
        
        return debugInfo;
    }
    
    // Определение имени браузера
    getBrowserName() {
        const ua = navigator.userAgent;
        if (ua.indexOf('OculusBrowser') > -1) return 'Oculus Browser';
        if (ua.indexOf('Chrome') > -1) return 'Chrome';
        if (ua.indexOf('Safari') > -1) return 'Safari';
        if (ua.indexOf('Firefox') > -1) return 'Firefox';
        if (ua.indexOf('Edge') > -1) return 'Edge';
        return 'Unknown';
    }
    
    // Отправка VR диагностики на сервер И показ на странице
    async sendVRDebugLog() {
        try {
            const debugInfo = await this.collectVRDebugInfo();
            
            console.log('Собрана VR debug информация:', debugInfo);
            
            // Показываем информацию на странице в VR
            this.showVRDebugPanel(debugInfo);
            
            // Отправляем на сервер для Serial Monitor
            const response = await fetch('/api/vr-log', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(debugInfo)
            });
            
            const result = await response.json();
            console.log('VR log отправлен на сервер:', result);
            
            return true;
        } catch (error) {
            console.error('Ошибка отправки VR log:', error);
            
            // Всё равно показываем информацию на странице
            const debugInfo = await this.collectVRDebugInfo();
            this.showVRDebugPanel(debugInfo, error.message);
            
            return false;
        }
    }
    
    // Показать debug панель в VR интерфейсе
    showVRDebugPanel(debugInfo, errorMessage = null) {
        const panel = document.getElementById('vrDebugPanel');
        const output = document.getElementById('vrDebugOutput');
        
        if (!panel || !output) return;
        
        // Форматируем информацию для удобного чтения
        let formattedInfo = '';
        
        // Основная информация
        formattedInfo += '=== BROWSER INFO ===\n';
        formattedInfo += `Browser: ${debugInfo.browser}\n`;
        formattedInfo += `User Agent: ${debugInfo.userAgent}\n`;
        formattedInfo += `Platform: ${debugInfo.platform}\n`;
        formattedInfo += `Screen: ${debugInfo.screenWidth}x${debugInfo.screenHeight}\n`;
        formattedInfo += `DPI: ${debugInfo.devicePixelRatio}\n\n`;
        
        // WebXR поддержка
        formattedInfo += '=== WEBXR SUPPORT ===\n';
        formattedInfo += `XR Supported: ${debugInfo.xrSupported ? 'YES ✓' : 'NO ✗'}\n`;
        if (debugInfo.xrSupported) {
            formattedInfo += `Immersive VR: ${debugInfo.immersiveVrSupported ? 'YES ✓' : 'NO ✗'}\n`;
            formattedInfo += `Immersive AR: ${debugInfo.immersiveArSupported ? 'YES ✓' : 'NO ✗'}\n`;
            formattedInfo += `Inline: ${debugInfo.inlineSupported ? 'YES ✓' : 'NO ✗'}\n`;
        }
        formattedInfo += '\n';
        
        // VR сессия
        formattedInfo += '=== VR SESSION ===\n';
        formattedInfo += `Session Active: ${debugInfo.vrSessionActive ? 'YES ✓' : 'NO ✗'}\n`;
        if (debugInfo.vrSession) {
            formattedInfo += `Blend Mode: ${debugInfo.vrSession.environmentBlendMode}\n`;
            formattedInfo += `Interaction: ${debugInfo.vrSession.interactionMode}\n`;
            formattedInfo += `Visibility: ${debugInfo.vrSession.visibilityState}\n`;
        }
        formattedInfo += '\n';
        
        // Контроллеры
        formattedInfo += '=== CONTROLLERS ===\n';
        formattedInfo += `Count: ${debugInfo.controllersCount}\n`;
        if (debugInfo.controllers && debugInfo.controllers.length > 0) {
            debugInfo.controllers.forEach((ctrl, idx) => {
                formattedInfo += `\nController ${idx + 1}:\n`;
                formattedInfo += `  Hand: ${ctrl.handedness}\n`;
                formattedInfo += `  Mode: ${ctrl.targetRayMode}\n`;
                formattedInfo += `  Profiles: ${ctrl.profiles.join(', ')}\n`;
                if (ctrl.gamepad) {
                    formattedInfo += `  Gamepad: ${ctrl.gamepad.id}\n`;
                    formattedInfo += `  Axes: [${ctrl.gamepad.axes.map(a => a.toFixed(2)).join(', ')}]\n`;
                    formattedInfo += `  Buttons: ${ctrl.gamepad.buttonsCount} (${ctrl.gamepad.buttons.filter(b => b.pressed).length} pressed)\n`;
                }
            });
        }
        formattedInfo += '\n';
        
        // Состояние приложения
        formattedInfo += '=== APP STATE ===\n';
        formattedInfo += `Device Type: ${debugInfo.app.deviceType}\n`;
        formattedInfo += `Control Mode: ${debugInfo.app.controlMode}\n`;
        formattedInfo += `Effect Mode: ${debugInfo.app.effectMode}\n`;
        formattedInfo += `Connected: ${debugInfo.app.isConnected ? 'YES ✓' : 'NO ✗'}\n`;
        formattedInfo += `VR Enabled: ${debugInfo.app.vrEnabled ? 'YES ✓' : 'NO ✗'}\n`;
        
        // Ошибка если есть
        if (errorMessage) {
            formattedInfo += '\n=== ERROR ===\n';
            formattedInfo += `Server Error: ${errorMessage}\n`;
            formattedInfo += '(Info shown locally only)\n';
        } else {
            formattedInfo += '\n✓ Sent to Serial Monitor (115200 baud)\n';
        }
        
        output.textContent = formattedInfo;
        panel.classList.remove('hidden');
    }
    
    // Скрыть debug панель
    hideVRDebugPanel() {
        const panel = document.getElementById('vrDebugPanel');
        if (panel) {
            panel.classList.add('hidden');
        }
    }
    
    // Переключить Live Log панель
    toggleVRLiveLog() {
        const panel = document.getElementById('vrLiveLogPanel');
        if (!panel) return;
        
        if (panel.classList.contains('hidden')) {
            // Показываем панель
            panel.classList.remove('hidden');
            
            // Включаем логирование на страницу
            Logger.enablePage('vrLiveLogOutput', true);
            Logger.vr('Live logging started');
            
            // Примеры логов для демонстрации
            Logger.info('VR Live Log активирован');
            Logger.debug('Этот лог будет виден в реальном времени');
        } else {
            // Скрываем панель
            this.hideVRLiveLog();
        }
    }
    
    // Скрыть Live Log панель
    hideVRLiveLog() {
        const panel = document.getElementById('vrLiveLogPanel');
        if (panel) {
            panel.classList.add('hidden');
            
            // Отключаем логирование на страницу (но оставляем в консоли)
            Logger.enablePage('vrLiveLogOutput', false);
            Logger.info('VR Live Log деактивирован');
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
        
        // Tab switching
        const tabButtons = document.querySelectorAll('.settings-tab');
        tabButtons.forEach(btn => {
            btn.addEventListener('click', (e) => {
                const targetTab = e.target.dataset.tab;
                
                // Validate tab exists
                if (!targetTab) return;
                
                // Remove active class from all tabs and panes
                document.querySelectorAll('.settings-tab').forEach(t => t.classList.remove('active'));
                document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));
                
                // Add active class to clicked tab and corresponding pane
                e.target.classList.add('active');
                const targetPane = document.getElementById(`tab-${targetTab}`);
                if (targetPane) {
                    targetPane.classList.add('active');
                }
            });
        });
        
        // Range slider value display
        const speedSlider = document.getElementById('speedSensitivity');
        const speedValue = document.getElementById('speedValue');
        if (speedSlider && speedValue) {
            speedSlider.addEventListener('input', (e) => {
                speedValue.textContent = e.target.value;
            });
        }
        
        const turnSlider = document.getElementById('turnSensitivity');
        const turnValue = document.getElementById('turnValue');
        if (turnSlider && turnValue) {
            turnSlider.addEventListener('input', (e) => {
                turnValue.textContent = e.target.value;
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
        
        // Обработчики для ручной загрузки .bin файлов закомментированы
        // Используется только метод обновления через GitHub
        /*
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
        */
        
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
        
        // Обработчики настройки моторов
        const saveMotorConfigBtn = document.getElementById('saveMotorConfig');
        if (saveMotorConfigBtn) {
            saveMotorConfigBtn.addEventListener('click', () => this.saveMotorConfig());
        }
        
        // Кнопка тестирования мотора (одна кнопка + радиокнопки для выбора)
        const testMotorBtn = document.getElementById('testMotorBtn');
        if (testMotorBtn) {
            testMotorBtn.addEventListener('click', () => {
                const selected = document.querySelector('input[name="testMotor"]:checked');
                if (selected) {
                    this.testMotor(selected.value);
                }
            });
        }
        
        // Help modal
        const helpModal = document.getElementById('helpModal');
        const helpCloseBtn = helpModal?.querySelector('.help-close');
        
        if (helpCloseBtn) {
            helpCloseBtn.addEventListener('click', () => {
                helpModal.classList.add('hidden');
                this.stopHelpAnimation();
            });
        }
        
        if (helpModal) {
            helpModal.addEventListener('click', (e) => {
                if (e.target === helpModal) {
                    helpModal.classList.add('hidden');
                    this.stopHelpAnimation();
                }
            });
        }
    }

    showHelp() {
        const modal = document.getElementById('helpModal');
        if (modal) {
            modal.classList.remove('hidden');
            this.startHelpAnimation();
        }
    }
    
    startHelpAnimation() {
        // Start animation for demo joysticks using requestAnimationFrame for better performance
        const leftKnob = document.getElementById('leftKnobDemo');
        const rightKnob = document.getElementById('rightKnobDemo');
        const leftXDisplay = document.getElementById('leftXDemo');
        const rightYDisplay = document.getElementById('rightYDemo');
        
        if (!leftKnob || !rightKnob) return;
        
        let time = 0;
        const maxHorizontalMove = 110; // Maximum horizontal movement in pixels
        const maxVerticalMove = 110;   // Maximum vertical movement in pixels
        
        const animate = () => {
            time += 0.015;
            
            // Left slider - horizontal only (in slot)
            const leftX = Math.sin(time) * maxHorizontalMove;
            leftKnob.style.transform = `translate(calc(-50% + ${leftX}px), -50%)`;
            const leftPercent = Math.round((leftX / maxHorizontalMove) * 100);
            if (leftXDisplay) leftXDisplay.textContent = leftPercent;
            
            // Right slider - vertical only (in slot)
            const rightY = Math.cos(time * 1.3) * maxVerticalMove;
            rightKnob.style.transform = `translate(-50%, calc(-50% + ${rightY}px))`;
            const rightPercent = Math.round((-rightY / maxVerticalMove) * 100);
            if (rightYDisplay) rightYDisplay.textContent = rightPercent;
            
            this.helpAnimationId = requestAnimationFrame(animate);
        };
        
        animate();
    }
    
    stopHelpAnimation() {
        if (this.helpAnimationId) {
            cancelAnimationFrame(this.helpAnimationId);
            this.helpAnimationId = null;
        }
    }

    showSettings() {
        const modal = document.getElementById('settingsModal');
        if (modal) {
            modal.classList.remove('hidden');
            
            // Загрузить текущие настройки
            const speedSlider = document.getElementById('speedSensitivity');
            const speedValue = document.getElementById('speedValue');
            if (speedSlider && speedValue) {
                speedSlider.value = this.speedSensitivity;
                speedValue.textContent = this.speedSensitivity;
            }
            
            const turnSlider = document.getElementById('turnSensitivity');
            const turnValue = document.getElementById('turnValue');
            if (turnSlider && turnValue) {
                turnSlider.value = this.turnSensitivity;
                turnValue.textContent = this.turnSensitivity;
            }
            
            // Установить текущий режим эффектов
            const effectRadio = document.querySelector('input[name="effectMode"][value="' + this.effectMode + '"]');
            if (effectRadio) {
                effectRadio.checked = true;
            }
            
            // Загрузить статус WiFi
            this.loadWiFiStatus();
            
            // Загрузить настройки моторов
            this.loadMotorConfig();
            
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
                
                // Store download URL and release info for later
                this.updateDownloadUrl = downloadUrl;
                this.latestReleaseInfo = {
                    version: latestVersion,
                    releaseName: releaseName,
                    releaseNotes: releaseNotes,
                    downloadUrl: downloadUrl
                };
                
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
    
    // Функция для показа экрана обновления прошивки с глитч-эффектом
    showFirmwareUpdateScreen(releaseInfo) {
        // Получаем существующий оверлей
        const overlay = document.getElementById('firmwareUpdateOverlay');
        if (!overlay) {
            console.error('Элемент firmwareUpdateOverlay не найден в DOM');
            return null;
        }
        
        // Заполняем информацию о релизе
        const versionEl = document.getElementById('firmwareVersion');
        const releaseNameEl = document.getElementById('firmwareReleaseName');
        const releaseNotesEl = document.getElementById('firmwareReleaseNotes');
        const glitchTextEl = document.getElementById('firmwareGlitchText');
        
        if (versionEl) {
            versionEl.innerHTML = `<strong>Версия:</strong> ${releaseInfo.version}`;
        }
        
        if (releaseNameEl) {
            releaseNameEl.innerHTML = `<strong>Релиз:</strong> ${releaseInfo.releaseName || 'Без названия'}`;
        }
        
        if (releaseNotesEl) {
            if (releaseInfo.releaseNotes) {
                releaseNotesEl.innerHTML = `<p><strong>Изменения:</strong></p><p style="font-size: 0.9em; opacity: 0.8; white-space: pre-wrap;">${releaseInfo.releaseNotes}</p>`;
            } else {
                releaseNotesEl.innerHTML = '';
            }
        }
        
        if (glitchTextEl) {
            glitchTextEl.textContent = 'ОБНОВЛЕНИЕ СИСТЕМЫ';
            glitchTextEl.classList.add('glitching');
        }
        
        // Сбрасываем прогресс
        this.updateFirmwareStatus('Подготовка к загрузке...', 0);
        
        // Добавляем глитч-эффект и показываем оверлей
        overlay.classList.add('glitch-effect');
        overlay.classList.remove('hidden');
        
        return overlay;
    }
    
    // Функция для обновления статуса на экране обновления
    updateFirmwareStatus(status, progress) {
        const statusEl = document.getElementById('firmwareStatus');
        const progressFill = document.getElementById('firmwareProgressFill');
        const progressText = document.getElementById('firmwareProgressText');
        
        if (statusEl) statusEl.textContent = status;
        if (progressFill) progressFill.style.width = progress + '%';
        if (progressText) progressText.textContent = progress + '%';
    }
    
    // Функция для скрытия оверлея с финальным глитч-эффектом и перезагрузкой страницы
    hideFirmwareUpdateScreen() {
        const overlay = document.getElementById('firmwareUpdateOverlay');
        if (overlay) {
            const glitchText = document.getElementById('firmwareGlitchText');
            if (glitchText) {
                glitchText.textContent = 'ПЕРЕЗАГРУЗКА...';
                glitchText.classList.add('glitching');
            }
            
            // Финальный глитч эффект перед скрытием
            setTimeout(() => {
                overlay.classList.add('glitch-effect');
                
                // Ждем пока устройство станет доступным, затем перезагружаем страницу
                this.waitForDeviceAndReload();
            }, 1000);
        }
    }
    
    // Функция для немедленного скрытия оверлея (при ошибке)
    closeFirmwareUpdateScreen() {
        const overlay = document.getElementById('firmwareUpdateOverlay');
        if (overlay) {
            overlay.classList.remove('glitch-effect');
            overlay.classList.add('hidden');
        }
    }
    
    async waitForDeviceAndReload() {
        console.log('Ожидание доступности устройства...');
        let attempts = 0;
        const maxAttempts = 30; // 30 попыток * 2 секунды = 60 секунд
        
        const checkInterval = setInterval(async () => {
            attempts++;
            
            try {
                // Пробуем получить текущую версию - это быстрый и надежный endpoint
                const response = await fetch('/api/update/current', { 
                    method: 'GET',
                    cache: 'no-cache'
                });
                
                if (response.ok) {
                    console.log('Устройство доступно! Перезагружаем страницу...');
                    clearInterval(checkInterval);
                    
                    // Небольшая задержка для эффекта
                    setTimeout(() => {
                        window.location.reload();
                    }, 500);
                }
            } catch (error) {
                console.log(`Попытка ${attempts}/${maxAttempts}: устройство еще не доступно`);
                
                if (attempts >= maxAttempts) {
                    console.log('Превышено время ожидания, перезагружаем страницу принудительно');
                    clearInterval(checkInterval);
                    window.location.reload();
                }
            }
        }, 2000);
    }

    async downloadAndInstallUpdate() {
        // Проверяем что есть информация о релизе
        if (!this.updateDownloadUrl || !this.latestReleaseInfo) {
            alert('URL обновления не найден. Сначала проверьте наличие обновлений.');
            return;
        }
        
        if (!confirm('Начать загрузку и установку обновления? Это займет несколько минут.')) {
            return;
        }
        
        Logger.info('Начало OTA обновления:', this.latestReleaseInfo.version);
        
        // Константы для состояний обновления (соответствуют UpdateState в FirmwareUpdate.h)
        const UpdateState = {
            IDLE: 0,
            DOWNLOADING: 1,
            UPLOADING: 2,
            SUCCESS: 3,
            FAILED: 4
        };
        
        // Константы для опроса статуса
        const POLL_INTERVAL_MS = 1000; // Опрос каждую секунду
        const TOTAL_TIMEOUT_MS = 120000; // Общий таймаут 2 минуты
        const MAX_CONSECUTIVE_ERRORS = 5; // Максимум последовательных ошибок в начале
        const MAX_CONSECUTIVE_ERRORS_LATE = 10; // Максимум последовательных ошибок в конце (при перезагрузке)
        const EARLY_ERROR_PERIOD_MS = 30000; // "Ранний" период - первые 30 секунд
        
        const maxPolls = TOTAL_TIMEOUT_MS / POLL_INTERVAL_MS;
        
        try {
            // ВАЖНО: Закрываем модальное окно настроек, если оно открыто
            const settingsModal = document.getElementById('settingsModal');
            if (settingsModal && !settingsModal.classList.contains('hidden')) {
                settingsModal.classList.add('hidden');
            }
            
            // Показываем экран обновления с глитч-эффектом
            const overlay = this.showFirmwareUpdateScreen(this.latestReleaseInfo);
            
            // Небольшая задержка для эффекта
            await new Promise(resolve => setTimeout(resolve, 1000));
            
            // Отправляем запрос на бэкенд для скачивания и установки
            this.updateFirmwareStatus('Отправка запроса на сервер...', 0);
            
            const formData = new FormData();
            formData.append('url', this.updateDownloadUrl);
            
            const response = await fetch('/api/update/download', {
                method: 'POST',
                body: formData
            });
            
            if (!response.ok) {
                const data = await response.json();
                throw new Error(data.message || 'Ошибка запуска обновления');
            }
            
            const data = await response.json();
            
            if (data.status === 'ok') {
                // Проверяем новый флаг rebooting
                if (data.rebooting) {
                    this.updateFirmwareStatus('Устройство перезагружается в безопасный режим...', 10);
                    
                    // Ждем перезагрузки и подключения
                    await new Promise(resolve => setTimeout(resolve, 5000));
                    
                    this.updateFirmwareStatus('Ожидание подключения устройства...', 15);
                    
                    // Пробуем подключиться к устройству
                    let reconnectAttempts = 0;
                    const maxReconnectAttempts = 30; // 30 попыток * 2 секунды = 60 секунд
                    
                    const checkConnection = setInterval(async () => {
                        reconnectAttempts++;
                        
                        try {
                            const statusResponse = await fetch('/api/update/status');
                            if (statusResponse.ok) {
                                clearInterval(checkConnection);
                                this.updateFirmwareStatus('Устройство подключено! Обновление в процессе...', 20);
                                
                                // Теперь начинаем обычный опрос статуса
                                this.pollUpdateStatus(overlay, UpdateState);
                            }
                        } catch (error) {
                            console.log('Reconnect attempt ' + reconnectAttempts);
                            this.updateFirmwareStatus('Ожидание подключения... (' + reconnectAttempts + '/' + maxReconnectAttempts + ')', 15 + (reconnectAttempts / maxReconnectAttempts * 5));
                            
                            if (reconnectAttempts >= maxReconnectAttempts) {
                                clearInterval(checkConnection);
                                throw new Error('Не удалось подключиться к устройству после перезагрузки');
                            }
                        }
                    }, 2000);
                    
                    return; // Выходим, pollUpdateStatus будет вызван после подключения
                }
                
                this.updateFirmwareStatus('Загрузка прошивки с GitHub...', 5);
                
                // Начинаем опрос статуса обновления (старый метод)
                this.pollUpdateStatus(overlay, UpdateState);
            } else {
                throw new Error(data.message || 'Неизвестная ошибка');
            }
            
        } catch (error) {
            console.error('Ошибка обновления:', error);
            this.updateFirmwareStatus('Ошибка: ' + error.message, 0);
            
            setTimeout(() => {
                this.closeFirmwareUpdateScreen();
                alert('Ошибка обновления: ' + error.message);
            }, 3000);
        }
    }
    
    pollUpdateStatus(overlay, UpdateState) {
        const POLL_INTERVAL_MS = 1000;
        const TOTAL_TIMEOUT_MS = 120000;
        const MAX_CONSECUTIVE_ERRORS = 5;
        const MAX_CONSECUTIVE_ERRORS_LATE = 10;
        const EARLY_ERROR_PERIOD_MS = 30000;
        
        const maxPolls = TOTAL_TIMEOUT_MS / POLL_INTERVAL_MS;
        
        let pollCount = 0;
        let consecutiveErrors = 0;
        let pollInterval = null;  // Declare outside try for cleanup in catch block
        
        try {
            pollInterval = setInterval(async () => {
                    pollCount++;
                    
                    try {
                        const statusResponse = await fetch('/api/update/status');
                        if (statusResponse.ok) {
                            const status = await statusResponse.json();
                            
                            // Сбрасываем счетчик ошибок при успешном ответе
                            consecutiveErrors = 0;
                            
                            // Обновляем прогресс на экране обновления
                            let statusText = status.status;
                            if (status.state === UpdateState.DOWNLOADING) {
                                statusText = 'Загрузка прошивки: ' + statusText;
                            } else if (status.state === UpdateState.UPLOADING) {
                                statusText = 'Установка прошивки: ' + statusText;
                            }
                            
                            this.updateFirmwareStatus(statusText, status.progress);
                            
                            // Проверяем состояние
                            if (status.state === UpdateState.SUCCESS) {
                                clearInterval(pollInterval);
                                this.updateFirmwareStatus('Обновление завершено! Перезагрузка...', 100);
                                
                                // Показываем финальный глитч и перезагружаем
                                setTimeout(() => {
                                    this.hideFirmwareUpdateScreen();
                                }, 1500);
                            } else if (status.state === UpdateState.FAILED) {
                                clearInterval(pollInterval);
                                throw new Error('Ошибка обновления: ' + status.status);
                            }
                        } else {
                            consecutiveErrors++;
                        }
                    } catch (error) {
                        consecutiveErrors++;
                        Logger.debug('Poll attempt', pollCount, 'error count:', consecutiveErrors, error.message);
                        
                        // Если слишком много последовательных ошибок в начале процесса - это проблема
                        const elapsedTimeMs = pollCount * POLL_INTERVAL_MS;
                        if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS && elapsedTimeMs < EARLY_ERROR_PERIOD_MS) {
                            clearInterval(pollInterval);
                            throw new Error('Не удалось получить статус обновления. Возможно, устройство недоступно.');
                        }
                        
                        // В конце процесса (когда уже идет перезагрузка) ошибки - это нормально
                        if (elapsedTimeMs > EARLY_ERROR_PERIOD_MS && consecutiveErrors >= MAX_CONSECUTIVE_ERRORS_LATE) {
                            // Считаем что обновление прошло успешно и устройство перезагружается
                            clearInterval(pollInterval);
                            this.updateFirmwareStatus('Устройство перезагружается...', 100);
                            setTimeout(() => {
                                this.hideFirmwareUpdateScreen();
                            }, 2000);
                        }
                    }
                    
                    // Таймаут
                    if (pollCount >= maxPolls) {
                        clearInterval(pollInterval);
                        alert('Превышено время ожидания. Устройство может перезагружаться. Проверьте статус вручную.');
                        this.closeFirmwareUpdateScreen();
                    }
                }, POLL_INTERVAL_MS);
        } catch (error) {
            console.error('Error during update polling:', error);
            
            // Clear interval if it was created
            if (pollInterval) {
                clearInterval(pollInterval);
            }
            
            this.updateFirmwareStatus('Ошибка: ' + error.message, 0);
            
            setTimeout(() => {
                this.closeFirmwareUpdateScreen();
                alert('Ошибка обновления: ' + error.message);
            }, 3000);
        }
    }
    
    // Функции для ручной загрузки .bin файлов закомментированы
    // Используется только метод обновления через GitHub
    /*
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
    */
    
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
            const now = Date.now();
            
            // Обработка геймпада (обновляет commandController.target*)
            this.processGamepad();
            
            // ВАЖНО: Command Controller - отправка команд с контролируемым интервалом
            // НЕ отправляем если уже идёт отправка (ждём завершения предыдущего запроса)
            if (!this.commandController.isSending) {
                const timeSinceSend = now - this.commandController.lastSendTime;
                const throttleChanged = Math.abs(this.commandController.targetThrottle - this.commandController.lastSentThrottle) > 20;
                const steeringChanged = Math.abs(this.commandController.targetSteering - this.commandController.lastSentSteering) > 20;
                const isStopCommand = (this.commandController.targetThrottle === 1500 && this.commandController.targetSteering === 1500);
                const wasMoving = (this.commandController.lastSentThrottle !== 1500 || this.commandController.lastSentSteering !== 1500);
                const isMovingNow = (this.commandController.targetThrottle !== 1500 || this.commandController.targetSteering !== 1500);
                
                // Отправляем если:
                // 1. Моторы активны И прошёл интервал (периодическая отправка для watchdog)
                // 2. ИЛИ значения изменились >20 PWM
                // 3. ИЛИ команда остановки после движения (обязательно отправляем стоп)
                const shouldSend = (isMovingNow && timeSinceSend >= this.commandController.sendInterval) || 
                                 throttleChanged || 
                                 steeringChanged ||
                                 (isStopCommand && wasMoving);
                
                if (shouldSend) {
                    this.commandController.isSending = true;
                    
                    // Используем быстрый GET запрос вместо POST
                    this.sendMoveCommand(
                        this.commandController.targetThrottle,
                        this.commandController.targetSteering
                    ).finally(() => {
                        this.commandController.isSending = false;
                    });
                    
                    this.commandController.lastSentThrottle = this.commandController.targetThrottle;
                    this.commandController.lastSentSteering = this.commandController.targetSteering;
                    this.commandController.lastSendTime = now;
                }
            }
            
            // Проверка соединения - каждые 5 секунд
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
    
    async loadMotorConfig() {
        try {
            const response = await fetch('/api/motor/config');
            if (response.ok) {
                const config = await response.json();
                
                // Устанавливаем значения в UI
                document.getElementById('motorSwapLeftRight').checked = config.motorSwapLeftRight || false;
                document.getElementById('motorInvertLeft').checked = config.motorInvertLeft || false;
                document.getElementById('motorInvertRight').checked = config.motorInvertRight || false;
                document.getElementById('invertThrottleStick').checked = config.invertThrottleStick || false;
                document.getElementById('invertSteeringStick').checked = config.invertSteeringStick || false;
                
                console.log('Настройки моторов загружены:', config);
            }
        } catch (error) {
            console.error('Ошибка загрузки настроек моторов:', error);
        }
    }
    
    async saveMotorConfig() {
        const config = {
            motorSwapLeftRight: document.getElementById('motorSwapLeftRight').checked,
            motorInvertLeft: document.getElementById('motorInvertLeft').checked,
            motorInvertRight: document.getElementById('motorInvertRight').checked,
            invertThrottleStick: document.getElementById('invertThrottleStick').checked,
            invertSteeringStick: document.getElementById('invertSteeringStick').checked
        };
        
        try {
            const response = await fetch('/api/motor/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(config)
            });
            
            if (response.ok) {
                const result = await response.json();
                alert('Настройки моторов сохранены! ' + result.message);
                console.log('Настройки моторов сохранены:', config);
            } else {
                alert('Ошибка сохранения настроек моторов');
            }
        } catch (error) {
            console.error('Ошибка сохранения настроек моторов:', error);
            alert('Ошибка сохранения настроек моторов');
        }
    }
    
    async testMotor(motor) {
        try {
            // Блокируем кнопку и радиокнопки на время теста
            const testBtn = document.getElementById('testMotorBtn');
            const radioButtons = document.querySelectorAll('input[name="testMotor"]');
            
            if (testBtn) {
                testBtn.disabled = true;
                testBtn.textContent = '⏳ Тестирование...';
            }
            radioButtons.forEach(radio => radio.disabled = true);
            
            // Определяем параметры для теста через /move
            let throttle, steering;
            if (motor === 'left') {
                // Левый мотор: газ вперёд + руль влево
                throttle = 2000; // Полный газ вперёд
                steering = 1000; // Полный влево
            } else {
                // Правый мотор: газ вперёд + руль вправо  
                throttle = 2000; // Полный газ вперёд
                steering = 2000; // Полный вправо
            }
            
            console.log(`Тест ${motor} мотора: /move?t=${throttle}&s=${steering}`);
            
            // Отправляем команду через GET /move
            await fetch(`/move?t=${throttle}&s=${steering}`);
            
            // Через 1 секунду останавливаем
            setTimeout(async () => {
                await fetch('/move?t=1500&s=1500');
                console.log('Тест завершён, моторы остановлены');
                
                // Разблокируем элементы
                if (testBtn) {
                    testBtn.disabled = false;
                    testBtn.textContent = '▶️ Запустить тест (1 сек)';
                }
                radioButtons.forEach(radio => radio.disabled = false);
            }, 1000);
            
        } catch (error) {
            console.error('Ошибка теста мотора:', error);
            alert('Ошибка теста мотора');
            
            // Разблокируем в случае ошибки
            const testBtn = document.getElementById('testMotorBtn');
            const radioButtons = document.querySelectorAll('input[name="testMotor"]');
            if (testBtn) {
                testBtn.disabled = false;
                testBtn.textContent = '▶️ Запустить тест (1 сек)';
            }
            radioButtons.forEach(radio => radio.disabled = false);
        }
    }
}

// Запуск при загрузке страницы
document.addEventListener('DOMContentLoaded', () => {
    window.microBoxController = new MicroBoxController();
    
    // Установить уровень логирования (можно менять в консоли: Logger.setLevel(Logger.LEVELS.DEBUG))
    // Logger.LEVELS: ERROR=0, WARN=1, INFO=2, DEBUG=3
    // Logger.setLevel(Logger.LEVELS.DEBUG); // Раскомментировать для отладки
});

// Предотвращение случайного закрытия
window.addEventListener('beforeunload', (e) => {
    e.preventDefault();
    e.returnValue = '';
});

Logger.info('МикроББокс система управления загружена');