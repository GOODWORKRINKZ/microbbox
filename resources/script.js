// ═══════════════════════════════════════════════════════════════
// МикРоББокс 0.1 - Клиентский код с SOLID, DRY, KISS принципами
// ═══════════════════════════════════════════════════════════════
// Архитектура:
// - Logger: Логирование (Single Responsibility)
// - CommandController: Управление командами (Single Responsibility)  
// - DeviceDetector: Определение типа устройства (Single Responsibility)
// - BaseRobotUI: Базовый класс UI для всех роботов (DRY)
// - ClassicRobotUI, LinerRobotUI, BrainRobotUI: Специфичные UI (Open/Closed)
// - RobotUIFactory: Фабрика для создания нужного UI (Factory Pattern)

'use strict';

// ═══════════════════════════════════════════════════════════════
// МОДУЛЬ ЛОГИРОВАНИЯ (Single Responsibility)
// ═══════════════════════════════════════════════════════════════

const Logger = {
    LEVELS: { ERROR: 0, WARN: 1, INFO: 2, DEBUG: 3 },
    currentLevel: 2,
    
    outputs: {
        console: true,
        api: false,
        page: false,
        pageElementId: null
    },
    
    pageBuffer: [],
    maxPageBuffer: 100,
    
    error(...args) {
        if (this.currentLevel >= this.LEVELS.ERROR) {
            this._log('ERROR', ...args);
        }
    },
    
    warn(...args) {
        if (this.currentLevel >= this.LEVELS.WARN) {
            this._log('WARN', ...args);
        }
    },
    
    info(...args) {
        if (this.currentLevel >= this.LEVELS.INFO) {
            this._log('INFO', ...args);
        }
    },
    
    debug(...args) {
        if (this.currentLevel >= this.LEVELS.DEBUG) {
            this._log('DEBUG', ...args);
        }
    },
    
    vr(...args) {
        this._log('VR', ...args);
    },
    
    _log(level, ...args) {
        const timestamp = new Date().toISOString().substring(11, 23);
        const message = args.map(arg => 
            typeof arg === 'object' ? JSON.stringify(arg) : String(arg)
        ).join(' ');
        
        const formattedMessage = `[${timestamp}] [${level}] ${message}`;
        
        if (this.outputs.console) {
            switch(level) {
                case 'ERROR': console.error(formattedMessage); break;
                case 'WARN': console.warn(formattedMessage); break;
                default: console.log(formattedMessage); break;
            }
        }
        
        if (this.outputs.page && this.outputs.pageElementId) {
            this._logToPage(formattedMessage);
        }
        
        if (this.outputs.api && (level === 'ERROR' || level === 'WARN' || level === 'VR')) {
            this._logToAPI(level, message);
        }
    },
    
    _logToPage(message) {
        this.pageBuffer.push(message);
        if (this.pageBuffer.length > this.maxPageBuffer) {
            this.pageBuffer.shift();
        }
        
        const element = document.getElementById(this.outputs.pageElementId);
        if (element) {
            element.textContent = this.pageBuffer.join('\n');
            element.scrollTop = element.scrollHeight;
        }
    },
    
    async _logToAPI(level, message) {
        try {
            const response = await fetch('/api/vr-log', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    level,
                    message,
                    timestamp: new Date().toISOString()
                })
            });
            
            if (!response.ok) {
                throw new Error(`API returned ${response.status}`);
            }
        } catch (error) {
            console.error('[Logger] Failed to send to API:', error.message);
        }
    },
    
    setLevel(level) {
        this.currentLevel = level;
    },
    
    enableConsole(enable = true) {
        this.outputs.console = enable;
    },
    
    enableAPI(enable = true) {
        this.outputs.api = enable;
    },
    
    enablePage(elementId, enable = true) {
        this.outputs.page = enable;
        this.outputs.pageElementId = elementId;
    },
    
    clearPageBuffer() {
        this.pageBuffer = [];
        const element = document.getElementById(this.outputs.pageElementId);
        if (element) {
            element.textContent = '';
        }
    },
    
    getPageLogs() {
        return this.pageBuffer.join('\n');
    }
};

// ═══════════════════════════════════════════════════════════════
// КОНТРОЛЛЕР КОМАНД (Single Responsibility)
// ═══════════════════════════════════════════════════════════════

class CommandController {
    constructor() {
        this.STOP_COMMAND_VALUE = 1500;  // Центральное положение PWM (остановка)
        this.targetThrottle = this.STOP_COMMAND_VALUE;
        this.targetSteering = this.STOP_COMMAND_VALUE;
        this.lastSentThrottle = this.STOP_COMMAND_VALUE;
        this.lastSentSteering = this.STOP_COMMAND_VALUE;
        this.lastSendTime = 0;
        this.sendInterval = 250;
        this.isSending = false;
        this.fetchTimeout = 200;
    }
    
    async loadConfig() {
        try {
            Logger.info('Загрузка конфигурации командного контроллера...');
            const response = await fetch('/api/config');
            if (!response.ok) {
                Logger.warn('Не удалось загрузить конфиг, используем значения по умолчанию');
                return;
            }            
            const config = await response.json();           
            
            Logger.info(`Конфигурация загружена: interval=${this.sendInterval}ms`);
        } catch (error) {
            Logger.error('Ошибка загрузки конфигурации:', error);
        }
    }
    
    setTarget(throttle, steering) {
        this.targetThrottle = throttle;
        this.targetSteering = steering;
    }
    
    async sendCommand() {
        if (this.isSending) return;
        
        const now = Date.now();
        
        // Определяем команду остановки и предыдущее движение
        const isStopCommand = (this.targetThrottle === this.STOP_COMMAND_VALUE && 
                               this.targetSteering === this.STOP_COMMAND_VALUE);
        const wasMoving = (this.lastSentThrottle !== this.STOP_COMMAND_VALUE || 
                          this.lastSentSteering !== this.STOP_COMMAND_VALUE);
        const isMoving = !isStopCommand;
        
        // Проверяем значительное изменение (>20 PWM)
        const throttleChange = Math.abs(this.targetThrottle - this.lastSentThrottle);
        const steeringChange = Math.abs(this.targetSteering - this.lastSentSteering);
        const significantChange = (throttleChange > 20 || steeringChange > 20);
        
        // Проверяем нужно ли отправлять команду
        const shouldSend = (
            // Всегда отправляем команду остановки после движения (предотвращает залипание)
            (isStopCommand && wasMoving) ||
            // Моторы активны И прошел интервал (watchdog)
            (isMoving && (now - this.lastSendTime >= this.sendInterval)) ||
            // Значения изменились >20 PWM
            significantChange
        );
        
        if (!shouldSend) return;
        
        this.isSending = true;
        
        try {
            const controller = new AbortController();
            const timeoutId = setTimeout(() => controller.abort(), this.fetchTimeout);
            
            const response = await fetch(`/move?t=${this.targetThrottle}&s=${this.targetSteering}`, {
                signal: controller.signal
            });
            
            clearTimeout(timeoutId);
            
            if (response.ok) {
                this.lastSentThrottle = this.targetThrottle;
                this.lastSentSteering = this.targetSteering;
                this.lastSendTime = now;
            }
        } catch (error) {
            if (error.name !== 'AbortError') {
                Logger.error('Ошибка отправки команды:', error);
            }
        } finally {
            this.isSending = false;
        }
    }
    
    stop() {
        this.setTarget(this.STOP_COMMAND_VALUE, this.STOP_COMMAND_VALUE);
    }
}

// ═══════════════════════════════════════════════════════════════
// ДЕТЕКТОР УСТРОЙСТВА (Single Responsibility)
// ═══════════════════════════════════════════════════════════════

class DeviceDetector {
    static detect() {
        const ua = navigator.userAgent.toLowerCase();
        const isOculusBrowser = ua.includes('oculusbrowser');
        const isMobile = /android|webos|iphone|ipad|ipod|blackberry|iemobile|opera mini/i.test(ua);
        
        if (isOculusBrowser) {
            return 'vr';
        } else if (isMobile) {
            return 'mobile';
        } else {
            return 'desktop';
        }
    }
    
    static getDeviceTypeText(deviceType) {
        switch (deviceType) {
            case 'vr': return '🥽 VR режим';
            case 'mobile': return '📱 Мобильное';
            case 'desktop': return '🖥️ ПК';
            default: return '❓ Неизвестно';
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// БАЗОВЫЙ КЛАСС UI РОБОТА (DRY - общая функциональность)
// ═══════════════════════════════════════════════════════════════

class BaseRobotUI {
    constructor() {
        this.GITHUB_REPO = 'GOODWORKRINKZ/microbbox';
        this.robotType = 'unknown';
        this.deviceType = DeviceDetector.detect();
        this.isConnected = false;
        this.commandController = new CommandController();
        
        // Состояние джойстиков
        this.leftJoystick = { x: 0, y: 0, active: false };
        this.rightJoystick = { x: 0, y: 0, active: false };
        
        // Настройки
        this.speedSensitivity = 80;
        this.turnSensitivity = 70;
        
        // VR
        this.xrSession = null;
        this.controllers = [];
    }
    
    async init() {
        Logger.info(`Инициализация ${this.robotType} UI...`);
        
        await this.loadRobotType();
        await this.commandController.loadConfig();
        
        this.setupInterface();
        this.setupCameraStream();
        this.setupEventListeners();
        
        await this.checkVRSupport();
        
        this.startMainLoop();
        
        // Скрыть загрузку
        setTimeout(() => {
            document.getElementById('loading')?.classList.add('hidden');
            document.getElementById('mainInterface')?.classList.remove('hidden');
        }, 2000);
        
        // Проверка версии и обновлений
        setTimeout(() => this.checkVersionAfterUpdate(), 3000);
        setTimeout(() => this.checkForUpdatesOnStartup(), 5000);
        
        Logger.info('Инициализация завершена');
    }
    
    async loadRobotType() {
        try {
            const response = await fetch('/api/robot-type');
            if (response.ok) {
                const data = await response.json();
                this.robotType = data.type || 'classic';
                Logger.info(`Тип робота: ${this.robotType}`);
            }
        } catch (error) {
            Logger.warn('Не удалось определить тип робота, используется classic');
            this.robotType = 'classic';
        }
    }
    
    setupInterface() {
        // Общая настройка интерфейса для всех типов
        const deviceTypeEl = document.getElementById('deviceType');
        if (deviceTypeEl) {
            deviceTypeEl.textContent = DeviceDetector.getDeviceTypeText(this.deviceType);
        }
        
        // Показываем нужные элементы управления
        this.showControlsForDevice();
    }
    
    showControlsForDevice() {
        const pcControls = document.getElementById('pcControls');
        const mobileControls = document.getElementById('mobileControls');
        const vrControls = document.getElementById('vrControls');
        const mobilePanel = document.querySelector('.mobile-panel');
        
        // Скрываем все
        [pcControls, mobileControls, vrControls].forEach(el => {
            if (el) el.classList.add('hidden');
        });
        
        // Показываем нужные
        switch (this.deviceType) {
            case 'desktop':
                if (pcControls) pcControls.classList.remove('hidden');
                // Показываем мобильную панель с кнопками для десктопа тоже
                if (mobilePanel) mobilePanel.classList.remove('hidden');
                break;
            case 'mobile':
                if (mobileControls) mobileControls.classList.remove('hidden');
                this.setupMobileJoysticks();
                break;
            case 'vr':
                if (vrControls) vrControls.classList.remove('hidden');
                break;
        }
    }
    
    setupCameraStream() {
        const streamImg = document.getElementById('cameraStream');
        if (streamImg) {
            // CameraServer работает на порту 81
            const streamUrl = `${window.location.protocol}//${window.location.hostname}:81/stream`;
            streamImg.src = streamUrl;
            streamImg.onerror = () => {
                Logger.error('Ошибка загрузки видео потока');
            };
        }
    }
    
    setupEventListeners() {
        // Полноэкранный режим
        const fullscreenBtn = document.getElementById('fullscreenBtn');
        if (fullscreenBtn) {
            fullscreenBtn.addEventListener('click', () => this.toggleFullscreen());
        }
        
        // Клавиатура для desktop
        if (this.deviceType === 'desktop') {
            this.setupKeyboardControls();
        }
        
        // Общие кнопки
        this.setupCommonButtons();
        
        // Настройки
        this.setupSettingsModal();
    }
    
    setupKeyboardControls() {
        document.addEventListener('keydown', (e) => this.handleKeyDown(e));
        document.addEventListener('keyup', (e) => this.handleKeyUp(e));
    }
    
    handleKeyDown(e) {
        const key = e.key.toLowerCase();
        
        // WASD или стрелки
        if (['w', 'arrowup', 's', 'arrowdown', 'a', 'arrowleft', 'd', 'arrowright'].includes(key)) {
            e.preventDefault();
            this.updateKeyboardControl(key, true);
        }
    }
    
    handleKeyUp(e) {
        const key = e.key.toLowerCase();
        
        if (['w', 'arrowup', 's', 'arrowdown', 'a', 'arrowleft', 'd', 'arrowright'].includes(key)) {
            e.preventDefault();
            this.updateKeyboardControl(key, false);
        }
    }
    
    updateKeyboardControl(key, pressed) {
        // Базовая реализация - может быть переопределена в наследниках
    }
    
    setupCommonButtons() {
        // Кнопки, которые есть у всех типов роботов
        // Используем универсальные мобильные кнопки для всех устройств
        const settingsBtn = document.getElementById('mobileSettings');
        if (settingsBtn) {
            settingsBtn.addEventListener('click', () => this.openSettings());
        }
        
        const helpBtn = document.getElementById('mobileHelp');
        if (helpBtn) {
            helpBtn.addEventListener('click', () => this.openHelp());
        }
    }
    
    setupSettingsModal() {
        // Настройка модального окна настроек
        const modal = document.getElementById('settingsModal');
        const closeBtn = modal?.querySelector('.close');
        
        if (closeBtn) {
            closeBtn.addEventListener('click', () => this.closeSettings());
        }
        
        // Табы
        const tabs = document.querySelectorAll('.settings-tab');
        tabs.forEach(tab => {
            tab.addEventListener('click', () => this.switchTab(tab.dataset.tab));
        });
        
        // Кнопки сохранения
        this.setupSaveButtons();
        
        // Настройка обработчиков обновления
        this.setupUpdateHandlers();
    }
    
    setupSaveButtons() {
        // Переопределяется в наследниках
    }
    
    setupUpdateHandlers() {
        // Кнопка проверки обновлений
        const checkBtn = document.getElementById('checkUpdatesBtn');
        if (checkBtn) {
            checkBtn.addEventListener('click', () => this.checkForUpdates());
        }
        
        // Кнопка скачивания обновления
        const downloadBtn = document.getElementById('downloadUpdateBtn');
        if (downloadBtn) {
            downloadBtn.addEventListener('click', () => this.downloadUpdate());
        }
        
        // Выбор типа робота
        const robotTypeRadios = document.querySelectorAll('input[name="robotType"]');
        const confirmBtn = document.getElementById('confirmRobotTypeBtn');
        
        robotTypeRadios.forEach(radio => {
            radio.addEventListener('change', () => {
                if (confirmBtn) {
                    confirmBtn.disabled = false;
                }
            });
        });
        
        if (confirmBtn) {
            confirmBtn.addEventListener('click', () => this.confirmRobotType());
        }
        
        // Чекбоксы настроек обновлений
        const autoUpdateCheck = document.getElementById('autoUpdate');
        const dontOfferCheck = document.getElementById('dontOfferUpdates');
        
        if (autoUpdateCheck) {
            autoUpdateCheck.addEventListener('change', () => this.saveUpdateSettings());
        }
        
        if (dontOfferCheck) {
            dontOfferCheck.addEventListener('change', () => this.saveUpdateSettings());
        }
        
        // Проверяем при открытии вкладки обновлений
        const updatesTab = document.querySelector('[data-tab="updates"]');
        if (updatesTab) {
            updatesTab.addEventListener('click', () => this.loadUpdateInfo());
        }
    }
    
    switchTab(tabName) {
        // Удаляем active со всех табов и панелей
        document.querySelectorAll('.settings-tab').forEach(t => t.classList.remove('active'));
        document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));
        
        // Добавляем active к нужным
        const tab = document.querySelector(`.settings-tab[data-tab="${tabName}"]`);
        const pane = document.getElementById(`tab-${tabName}`);
        
        if (tab) tab.classList.add('active');
        if (pane) pane.classList.add('active');
    }
    
    openSettings() {
        const modal = document.getElementById('settingsModal');
        if (modal) {
            modal.classList.remove('hidden');
            this.loadSettings();
        }
    }
    
    closeSettings() {
        const modal = document.getElementById('settingsModal');
        if (modal) {
            modal.classList.add('hidden');
        }
    }
    
    openHelp() {
        const modal = document.getElementById('helpModal');
        if (modal) {
            modal.classList.remove('hidden');
        }
    }
    
    async loadSettings() {
        // Загрузка настроек с сервера
        // Переопределяется в наследниках
    }
    
    setupMobileJoysticks() {
        const leftJoy = document.getElementById('leftJoystick');
        const rightJoy = document.getElementById('rightJoystick');
        
        if (leftJoy) this.setupJoystick(leftJoy, 'left');
        if (rightJoy) this.setupJoystick(rightJoy, 'right');
    }
    
    setupJoystick(element, side) {
        // Реализация сенсорного джойстика
        // Упрощенная версия - полная реализация в конкретных типах роботов
        Logger.debug(`Настройка джойстика: ${side}`);
    }
    
    async checkVRSupport() {
        if (!navigator.xr) {
            Logger.debug('WebXR не поддерживается');
            return;
        }
        
        try {
            const supported = await navigator.xr.isSessionSupported('immersive-vr');
            if (supported) {
                Logger.info('VR поддерживается');
                const vrBtn = document.getElementById('vrBtn');
                if (vrBtn) {
                    vrBtn.classList.remove('hidden');
                    vrBtn.addEventListener('click', () => this.enterVR());
                }
            }
        } catch (error) {
            Logger.debug('Ошибка проверки VR поддержки:', error);
        }
    }
    
    async enterVR() {
        Logger.info('Вход в VR режим...');
        // Переопределяется в наследниках
    }
    
    startMainLoop() {
        setInterval(() => this.mainLoop(), 50); // 20 Hz
    }
    
    mainLoop() {
        // Отправка команд
        this.commandController.sendCommand();
        
        // Дополнительная логика в наследниках
        this.updateSpecific();
    }
    
    updateSpecific() {
        // Переопределяется в наследниках
    }
    
    toggleFullscreen() {
        if (!document.fullscreenElement) {
            document.documentElement.requestFullscreen();
        } else {
            document.exitFullscreen();
        }
    }
    
    async checkVersionAfterUpdate() {
        try {
            const response = await fetch('/api/update/current');
            if (!response.ok) return;
            
            const data = await response.json();
            const currentVersion = data.version;
            
            if (!currentVersion) return;
            
            const savedVersion = localStorage.getItem('microbbox_version');
            
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
        // Показ уведомления об успешном обновлении
        Logger.info(`Показ уведомления: ${oldVersion} -> ${newVersion}`);
        // Реализация в конкретных типах при необходимости
    }
    
    async checkForUpdatesOnStartup() {
        try {
            const settingsResponse = await fetch('/api/update/settings');
            if (!settingsResponse.ok) return;
            
            const settings = await settingsResponse.json();
            if (!settings.autoUpdate || settings.dontOffer) {
                return;
            }
            
            const response = await fetch('/api/update/check');
            if (response.ok) {
                const data = await response.json();
                
                if (data.hasUpdate) {
                    const message = `Доступна новая версия ${data.version}. Обновить сейчас?`;
                    if (confirm(message)) {
                        // Открываем настройки на вкладке обновлений
                        setTimeout(() => {
                            this.openSettings();
                            const updateSection = document.querySelector('[data-tab="updates"]');
                            if (updateSection) {
                                updateSection.click();
                            }
                        }, 500);
                    }
                }
            }
        } catch (error) {
            Logger.debug('Не удалось проверить обновления:', error);
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // МЕТОДЫ ОБНОВЛЕНИЯ ПРОШИВКИ (ТОЛЬКО КЛИЕНТ)
    // ═══════════════════════════════════════════════════════════════
    
    async loadUpdateInfo() {
        try {
            // Загружаем текущую версию
            const currentResponse = await fetch('/api/update/current');
            if (currentResponse.ok) {
                const data = await currentResponse.json();
                const versionEl = document.getElementById('currentVersion');
                const releaseNameEl = document.getElementById('releaseName');
                if (versionEl) versionEl.textContent = data.version;
                if (releaseNameEl) releaseNameEl.textContent = data.releaseName;
                
                // Показываем выбор типа робота только для старых версий
                this.checkIfNeedsRobotTypeSelection(data.version);
            }
            
            // Загружаем настройки
            const settingsResponse = await fetch('/api/update/settings');
            if (settingsResponse.ok) {
                const settings = await settingsResponse.json();
                const autoUpdateCheck = document.getElementById('autoUpdate');
                const dontOfferCheck = document.getElementById('dontOfferUpdates');
                if (autoUpdateCheck) autoUpdateCheck.checked = settings.autoUpdate;
                if (dontOfferCheck) dontOfferCheck.checked = settings.dontOffer;
            }
        } catch (error) {
            Logger.error('Ошибка загрузки информации об обновлениях:', error);
        }
    }
    
    checkIfNeedsRobotTypeSelection(version) {
        // Показываем выбор типа только для версий 0.0.X
        const needsSelection = version && (version.startsWith('v0.0.') || version.startsWith('0.0.'));
        const selectionDiv = document.getElementById('robotTypeSelection');
        
        if (needsSelection && selectionDiv) {
            selectionDiv.classList.remove('hidden');
            // Загружаем сохраненный выбор из localStorage
            const savedType = localStorage.getItem('selectedRobotType');
            if (savedType) {
                const radio = document.querySelector(`input[name="robotType"][value="${savedType}"]`);
                if (radio) radio.checked = true;
                const confirmBtn = document.getElementById('confirmRobotTypeBtn');
                if (confirmBtn) confirmBtn.disabled = false;
            }
        } else if (selectionDiv) {
            selectionDiv.classList.add('hidden');
        }
    }
    
    confirmRobotType() {
        const selectedRadio = document.querySelector('input[name="robotType"]:checked');
        if (!selectedRadio) return;
        
        const robotType = selectedRadio.value;
        
        // Сохраняем выбор в localStorage (только клиентская сторона)
        localStorage.setItem('selectedRobotType', robotType);
        this.selectedRobotType = robotType;
        
        Logger.info('Тип робота выбран:', robotType);
        alert(`Выбран тип устройства: ${this.getRobotTypeName(robotType)}.\nТеперь можно проверить обновления.`);
    }
    
    getRobotTypeName(type) {
        const names = {
            'classic': 'МикроБокс Классик',
            'liner': 'МикроБокс Лайнер',
            'brain': 'МикроБокс Брейн'
        };
        return names[type] || type;
    }
    
    async checkForUpdates() {
        const checkBtn = document.getElementById('checkUpdatesBtn');
        if (checkBtn) checkBtn.disabled = true;
        
        // Проверяем, выбран ли тип робота (для старых версий)
        const currentVersion = document.getElementById('currentVersion')?.textContent || '';
        const needsSelection = currentVersion && (currentVersion.startsWith('v0.0.') || currentVersion.startsWith('0.0.'));
        
        if (needsSelection) {
            const savedType = localStorage.getItem('selectedRobotType');
            if (!savedType) {
                alert('Сначала выберите тип устройства');
                if (checkBtn) checkBtn.disabled = false;
                return;
            }
            this.selectedRobotType = savedType;
        }
        
        try {
            const response = await fetch('/api/update/check');
            if (!response.ok) {
                alert('Ошибка проверки обновлений');
                return;
            }
            
            const data = await response.json();
            const updateAvailableDiv = document.getElementById('updateAvailable');
            
            if (data.hasUpdate) {
                document.getElementById('newVersion').textContent = data.version;
                document.getElementById('newReleaseName').textContent = data.releaseName;
                document.getElementById('releaseNotes').textContent = data.releaseNotes;
                
                // Модифицируем URL если выбран тип робота
                let downloadUrl = data.downloadUrl;
                if (this.selectedRobotType && needsSelection) {
                    // Заменяем URL на специфичный для типа робота
                    // Пример: microbox-classic-v0.1.0-release.bin
                    const version = data.version;
                    downloadUrl = downloadUrl.replace(/microbox-.*?-release\.bin/, 
                        `microbox-${this.selectedRobotType}-${version}-release.bin`);
                    Logger.info(`Используется URL для типа ${this.selectedRobotType}: ${downloadUrl}`);
                }
                
                if (updateAvailableDiv) updateAvailableDiv.classList.remove('hidden');
                
                // Сохраняем модифицированный URL для скачивания
                this.updateDownloadUrl = downloadUrl;
            } else {
                if (updateAvailableDiv) updateAvailableDiv.classList.add('hidden');
                alert('У вас установлена последняя версия');
            }
        } catch (error) {
            Logger.error('Ошибка проверки обновлений:', error);
            alert('Ошибка подключения к серверу обновлений');
        } finally {
            if (checkBtn) checkBtn.disabled = false;
        }
    }
    
    async downloadUpdate() {
        if (!this.updateDownloadUrl) {
            alert('URL для скачивания не найден');
            return;
        }
        
        const confirmed = confirm('Начать обновление прошивки?\nУстройство перезагрузится после завершения загрузки.');
        if (!confirmed) return;
        
        try {
            // Показываем оверлей обновления
            this.showFirmwareUpdateOverlay();
            
            const response = await fetch('/api/update/download', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: `url=${encodeURIComponent(this.updateDownloadUrl)}`
            });
            
            if (response.ok) {
                const data = await response.json();
                if (data.rebooting) {
                    document.getElementById('firmwareStatus').textContent = 'Устройство перезагружается...';
                    // Устройство перезагружается, показываем индикатор
                    setTimeout(() => {
                        document.getElementById('firmwareStatus').textContent = 'Ожидание перезагрузки...';
                    }, 3000);
                }
            } else {
                alert('Ошибка запуска обновления');
                this.hideFirmwareUpdateOverlay();
            }
        } catch (error) {
            Logger.error('Ошибка загрузки обновления:', error);
            alert('Ошибка подключения к устройству');
            this.hideFirmwareUpdateOverlay();
        }
    }
    
    async saveUpdateSettings() {
        const autoUpdate = document.getElementById('autoUpdate')?.checked || false;
        const dontOffer = document.getElementById('dontOfferUpdates')?.checked || false;
        
        try {
            await fetch('/api/update/settings', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ autoUpdate, dontOffer })
            });
        } catch (error) {
            Logger.error('Ошибка сохранения настроек обновлений:', error);
        }
    }
    
    showFirmwareUpdateOverlay() {
        const overlay = document.getElementById('firmwareUpdateOverlay');
        if (overlay) overlay.classList.remove('hidden');
    }
    
    hideFirmwareUpdateOverlay() {
        const overlay = document.getElementById('firmwareUpdateOverlay');
        if (overlay) overlay.classList.add('hidden');
    }
}


// ═══════════════════════════════════════════════════════════════
// CLASSIC ROBOT UI - Управляемый робот
// ═══════════════════════════════════════════════════════════════

class ClassicRobotUI extends BaseRobotUI {
    constructor() {
        super();
        this.robotType = 'classic';
        this.effectMode = 'normal';
        this.keyStates = {};
        
        // T-800 overlay
        this.t800Interval = null;
        this.t800StartTime = null;
    }
    
    setupEventListeners() {
        super.setupEventListeners();
        
        // Кнопки эффектов
        const effectModeSelect = document.getElementById('effectMode');
        if (effectModeSelect) {
            effectModeSelect.addEventListener('change', (e) => {
                this.setEffectMode(e.target.value);
            });
        }
        
        // Фонарик
        const flashlightBtn = document.getElementById('flashlightBtn');
        if (flashlightBtn) {
            flashlightBtn.addEventListener('click', () => this.toggleFlashlight());
        }
        
        // Сигнал
        const hornBtn = document.getElementById('hornBtn');
        if (hornBtn) {
            hornBtn.addEventListener('click', () => this.playHorn());
        }
        
        // Кнопки управления для ПК
        document.querySelectorAll('.control-btn').forEach(btn => {
            btn.addEventListener('mousedown', () => {
                const direction = btn.dataset.direction;
                this.handleControlButton(direction, true);
            });
            
            btn.addEventListener('mouseup', () => {
                const direction = btn.dataset.direction;
                this.handleControlButton(direction, false);
            });
        });
    }
    
    handleControlButton(direction, pressed) {
        if (!pressed) {
            this.commandController.stop();
            return;
        }
        
        const speedMap = {
            'forward': { t: 2000, s: 1500 },
            'backward': { t: 1000, s: 1500 },
            'left': { t: 1500, s: 1000 },
            'right': { t: 1500, s: 2000 },
            'stop': { t: 1500, s: 1500 }
        };
        
        const speed = speedMap[direction];
        if (speed) {
            this.commandController.setTarget(speed.t, speed.s);
        }
    }
    
    updateKeyboardControl(key, pressed) {
        this.keyStates[key] = pressed;
        
        let throttle = 1500;
        let steering = 1500;
        
        // Расчет throttle
        if (this.keyStates['w'] || this.keyStates['arrowup']) {
            throttle = 2000;
        } else if (this.keyStates['s'] || this.keyStates['arrowdown']) {
            throttle = 1000;
        }
        
        // Расчет steering
        if (this.keyStates['a'] || this.keyStates['arrowleft']) {
            steering = 1000;
        } else if (this.keyStates['d'] || this.keyStates['arrowright']) {
            steering = 2000;
        }
        
        this.commandController.setTarget(throttle, steering);
    }
    
    async setEffectMode(mode) {
        this.effectMode = mode;
        
        try {
            await fetch(`/effect?mode=${mode}`);
            
            // T-800 overlay
            if (mode === 'terminator') {
                this.startT800Overlay();
            } else {
                this.stopT800Overlay();
            }
        } catch (error) {
            Logger.error('Ошибка установки эффекта:', error);
        }
    }
    
    startT800Overlay() {
        const overlay = document.getElementById('t800Overlay');
        if (!overlay) return;
        
        overlay.classList.remove('hidden');
        this.t800StartTime = Date.now();
        
        this.t800Interval = setInterval(() => {
            const elapsed = Math.floor((Date.now() - this.t800StartTime) / 1000);
            const hours = Math.floor(elapsed / 3600);
            const minutes = Math.floor((elapsed % 3600) / 60);
            const seconds = elapsed % 60;
            
            document.getElementById('t800Time').textContent = 
                `${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
            
            // Случайные значения для реализма
            document.getElementById('t800Mem').textContent = 
                '0x' + Math.floor(Math.random() * 0xFFFF).toString(16).toUpperCase();
            document.getElementById('t800Power').textContent = 
                (98 + Math.random() * 2).toFixed(1) + '%';
            document.getElementById('t800Temp').textContent = 
                (36 + Math.random() * 2).toFixed(1) + '°C';
        }, 1000);
    }
    
    stopT800Overlay() {
        const overlay = document.getElementById('t800Overlay');
        if (overlay) {
            overlay.classList.add('hidden');
        }
        
        if (this.t800Interval) {
            clearInterval(this.t800Interval);
            this.t800Interval = null;
        }
    }
    
    async toggleFlashlight() {
        try {
            await fetch('/flashlight');
        } catch (error) {
            Logger.error('Ошибка переключения фонарика:', error);
        }
    }
    
    async playHorn() {
        try {
            await fetch('/horn');
        } catch (error) {
            Logger.error('Ошибка воспроизведения сигнала:', error);
        }
    }
    
    setupJoystick(element, side) {
        // Полная реализация джойстика для Classic
        const knob = element.querySelector('.joystick-knob');
        let isDragging = false;
        let touchId = null;
        
        // Knob padding from edges - accounts for 50px knob diameter
        const KNOB_EDGE_PADDING = 30;
        
        const handleStart = (clientX, clientY, id = null) => {
            isDragging = true;
            touchId = id;
        };
        
        const handleMove = (clientX, clientY) => {
            if (!isDragging) return;
            
            const rect = element.getBoundingClientRect();
            const centerX = rect.width / 2;
            const centerY = rect.height / 2;
            
            let deltaX = clientX - rect.left - centerX;
            let deltaY = clientY - rect.top - centerY;
            
            // Constrain movement based on joystick type
            // Left joystick (rotation): horizontal movement only (X-axis)
            // Right joystick (gas/throttle): vertical movement only (Y-axis)
            if (side === 'left') {
                // Horizontal slot - only X-axis movement allowed
                deltaY = 0;
                const maxDistance = rect.width / 2 - KNOB_EDGE_PADDING;
                deltaX = Math.max(-maxDistance, Math.min(maxDistance, deltaX));
            } else {
                // Vertical slot - only Y-axis movement allowed
                deltaX = 0;
                const maxDistance = rect.height / 2 - KNOB_EDGE_PADDING;
                deltaY = Math.max(-maxDistance, Math.min(maxDistance, deltaY));
            }
            
            knob.style.transform = `translate(calc(-50% + ${deltaX}px), calc(-50% + ${deltaY}px))`;
            
            // Calculate percentages based on actual max distance for each axis
            const maxDistanceX = side === 'left' ? (rect.width / 2 - KNOB_EDGE_PADDING) : 0;
            const maxDistanceY = side === 'right' ? (rect.height / 2 - KNOB_EDGE_PADDING) : 0;
            
            const percentX = maxDistanceX > 0 ? (deltaX / maxDistanceX) * 100 : 0;
            const percentY = maxDistanceY > 0 ? (-deltaY / maxDistanceY) * 100 : 0;
            
            if (side === 'left') {
                this.leftJoystick = { x: percentX, y: 0, active: true };
            } else {
                this.rightJoystick = { x: 0, y: percentY, active: true };
            }
            
            this.updateMotorFromJoysticks();
        };
        
        const handleEnd = () => {
            isDragging = false;
            touchId = null;
            
            knob.style.transform = 'translate(-50%, -50%)';
            
            if (side === 'left') {
                this.leftJoystick = { x: 0, y: 0, active: false };
            } else {
                this.rightJoystick = { x: 0, y: 0, active: false };
            }
            
            this.updateMotorFromJoysticks();
        };
        
        // Mouse события
        element.addEventListener('mousedown', (e) => {
            e.preventDefault();
            handleStart(e.clientX, e.clientY);
        });
        
        document.addEventListener('mousemove', (e) => {
            if (isDragging) {
                e.preventDefault();
                handleMove(e.clientX, e.clientY);
            }
        });
        
        document.addEventListener('mouseup', () => {
            if (isDragging) {
                handleEnd();
            }
        });
        
        // Touch события
        element.addEventListener('touchstart', (e) => {
            e.preventDefault();
            const touch = e.touches[0];
            handleStart(touch.clientX, touch.clientY, touch.identifier);
        });
        
        element.addEventListener('touchmove', (e) => {
            if (!isDragging) return;
            e.preventDefault();
            
            for (let i = 0; i < e.touches.length; i++) {
                if (e.touches[i].identifier === touchId) {
                    const touch = e.touches[i];
                    handleMove(touch.clientX, touch.clientY);
                    break;
                }
            }
        });
        
        element.addEventListener('touchend', (e) => {
            if (!isDragging) return;
            e.preventDefault();
            
            for (let i = 0; i < e.changedTouches.length; i++) {
                if (e.changedTouches[i].identifier === touchId) {
                    handleEnd();
                    break;
                }
            }
        });
    }
    
    updateMotorFromJoysticks() {
        // Дифференциальное управление
        const speed = this.rightJoystick.y * this.speedSensitivity / 100;
        const turn = this.leftJoystick.x * this.turnSensitivity / 100;
        
        // Преобразование в PWM (1000-2000)
        const throttle = 1500 + (speed * 5);
        const steering = 1500 + (turn * 5);
        
        this.commandController.setTarget(
            Math.round(Math.max(1000, Math.min(2000, throttle))),
            Math.round(Math.max(1000, Math.min(2000, steering)))
        );
    }
}

// ═══════════════════════════════════════════════════════════════
// LINER ROBOT UI - Автономный робот следующий по линии
// ═══════════════════════════════════════════════════════════════
// Liner идентичен Classic - переключение режима через физическую кнопку на GPIO4

class LinerRobotUI extends ClassicRobotUI {
    constructor() {
        super();
        this.robotType = 'liner';
    }
    
    // Liner полностью идентичен Classic UI
    // Переключение автономного режима происходит через физическую кнопку на GPIO4
    // Никаких дополнительных UI элементов не требуется
}

// ═══════════════════════════════════════════════════════════════
// BRAIN ROBOT UI - Модуль управления для других роботов
// ═══════════════════════════════════════════════════════════════
// Brain идентичен Classic - транслирует команды через API

class BrainRobotUI extends ClassicRobotUI {
    constructor() {
        super();
        this.robotType = 'brain';
    }
    
    // Brain полностью идентичен Classic UI
    // Транслирует команды управления через API в другие протоколы (PWM/PPM/SBUS/TBS)
    // Никаких дополнительных UI элементов не требуется
}

// ═══════════════════════════════════════════════════════════════
// ФАБРИКА UI (Factory Pattern)
// ═══════════════════════════════════════════════════════════════

class RobotUIFactory {
    static create(robotType) {
        switch (robotType) {
            case 'classic':
                return new ClassicRobotUI();
            case 'liner':
                return new LinerRobotUI();
            case 'brain':
                return new BrainRobotUI();
            default:
                Logger.warn(`Неизвестный тип робота: ${robotType}, используется classic`);
                return new ClassicRobotUI();
        }
    }
    
    static async createFromServer() {
        try {
            const response = await fetch('/api/robot-type');
            if (response.ok) {
                const data = await response.json();
                return RobotUIFactory.create(data.type || 'classic');
            }
        } catch (error) {
            Logger.warn('Не удалось определить тип робота с сервера, используется classic');
        }
        
        return new ClassicRobotUI();
    }
}

// ═══════════════════════════════════════════════════════════════
// ИНИЦИАЛИЗАЦИЯ
// ═══════════════════════════════════════════════════════════════

document.addEventListener('DOMContentLoaded', async () => {
    Logger.info('МикРоББокс 0.1 загружается...');
    
    // Создаем нужный UI через фабрику
    window.robotUI = await RobotUIFactory.createFromServer();
    
    // Инициализируем
    await window.robotUI.init();
    
    Logger.info('МикРоББокс 0.1 готов!');
});

// Предотвращение случайного закрытия
window.addEventListener('beforeunload', (e) => {
    e.preventDefault();
    e.returnValue = '';
});
