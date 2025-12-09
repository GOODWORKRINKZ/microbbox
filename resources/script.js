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
// МОДУЛЬ МОДАЛЬНЫХ ДИАЛОГОВ (Single Responsibility)
// Замена нативных alert() и confirm() на красивые модальные окна
// ═══════════════════════════════════════════════════════════════

const ModalDialog = {
    /**
     * Показывает модальное окно с сообщением (замена alert)
     * @param {string} message - Текст сообщения
     * @param {string} title - Заголовок окна (необязательно)
     * @returns {Promise<void>}
     */
    async showAlert(message, title = '⚠️ Уведомление') {
        return new Promise((resolve) => {
            // Создаем модальное окно
            const modal = document.createElement('div');
            modal.className = 'modal modal-dialog';
            modal.innerHTML = `
                <div class="modal-content modal-alert">
                    <h2>${title}</h2>
                    <p class="modal-message">${message}</p>
                    <div class="modal-buttons">
                        <button class="btn-primary modal-btn-ok">OK</button>
                    </div>
                </div>
            `;
            
            document.body.appendChild(modal);
            
            // Показываем модальное окно с анимацией
            requestAnimationFrame(() => {
                modal.style.display = 'flex';
            });
            
            // Обработчик кнопки OK
            const okBtn = modal.querySelector('.modal-btn-ok');
            okBtn.addEventListener('click', () => {
                modal.remove();
                resolve();
            });
            
            // Закрытие по клику на фон
            modal.addEventListener('click', (e) => {
                if (e.target === modal) {
                    modal.remove();
                    resolve();
                }
            });
            
            // Закрытие по Escape
            const escapeHandler = (e) => {
                if (e.key === 'Escape') {
                    modal.remove();
                    document.removeEventListener('keydown', escapeHandler);
                    resolve();
                }
            };
            document.addEventListener('keydown', escapeHandler);
        });
    },
    
    /**
     * Показывает модальное окно подтверждения (замена confirm)
     * @param {string} message - Текст вопроса
     * @param {string} title - Заголовок окна (необязательно)
     * @returns {Promise<boolean>} - true если подтвердили, false если отменили
     */
    async showConfirm(message, title = '❓ Подтверждение') {
        return new Promise((resolve) => {
            // Создаем модальное окно
            const modal = document.createElement('div');
            modal.className = 'modal modal-dialog';
            modal.innerHTML = `
                <div class="modal-content modal-confirm">
                    <h2>${title}</h2>
                    <p class="modal-message">${message}</p>
                    <div class="modal-buttons">
                        <button class="btn-secondary modal-btn-cancel">Отмена</button>
                        <button class="btn-primary modal-btn-confirm">Подтвердить</button>
                    </div>
                </div>
            `;
            
            document.body.appendChild(modal);
            
            // Показываем модальное окно с анимацией
            requestAnimationFrame(() => {
                modal.style.display = 'flex';
            });
            
            // Обработчик кнопки Подтвердить
            const confirmBtn = modal.querySelector('.modal-btn-confirm');
            confirmBtn.addEventListener('click', () => {
                modal.remove();
                resolve(true);
            });
            
            // Обработчик кнопки Отмена
            const cancelBtn = modal.querySelector('.modal-btn-cancel');
            cancelBtn.addEventListener('click', () => {
                modal.remove();
                resolve(false);
            });
            
            // Закрытие по клику на фон = отмена
            modal.addEventListener('click', (e) => {
                if (e.target === modal) {
                    modal.remove();
                    resolve(false);
                }
            });
            
            // Закрытие по Escape = отмена
            const escapeHandler = (e) => {
                if (e.key === 'Escape') {
                    modal.remove();
                    document.removeEventListener('keydown', escapeHandler);
                    resolve(false);
                }
            };
            document.addEventListener('keydown', escapeHandler);
        });
    }
};

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
// ДЕТЕКТОР УСТРОЙСТВА - ВСЕГДА МОБИЛЬНЫЙ РЕЖИМ
// ═══════════════════════════════════════════════════════════════

class DeviceDetector {
    static detect() {
        // Всегда возвращаем mobile для унифицированного интерфейса
        return 'mobile';
    }
    
    static getDeviceTypeText(deviceType) {
        // Всегда мобильный интерфейс
        return '📱 Мобильное управление';
    }
}

// ═══════════════════════════════════════════════════════════════
// БАЗОВЫЙ КЛАСС UI РОБОТА (DRY - общая функциональность)
// ═══════════════════════════════════════════════════════════════

class BaseRobotUI {
    // Статические константы класса
    static ROBOT_TYPES = ['classic', 'liner', 'brain']; // Доступные типы роботов
    
    // Конфигурация стрима (DRY) - инициализируется один раз
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
        
        // Конфигурация стрима (один раз)
        this.STREAM_CONFIG = {
            PORT: 81,
            MAX_RECONNECT_ATTEMPTS: 10,
            BASE_RECONNECT_DELAY: 2000,
            MAX_RECONNECT_DELAY: 30000,  // Максимум 30 секунд
            ERROR_DEBOUNCE: 1000
        };
    }
    
    async init() {
        Logger.info(`Инициализация ${this.robotType} UI...`);
        
        await this.loadRobotType();
        await this.commandController.loadConfig();
        
        this.setupInterface();
        this.setupCameraStream();
        this.setupEventListeners();
        
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
        const mobileControls = document.getElementById('mobileControls');
        
        // Всегда показываем только мобильные контролы
        if (mobileControls) {
            mobileControls.classList.remove('hidden');
            this.setupMobileJoysticks();
        }
    }
    
    // DRY: Единственное место для формирования URL стрима
    getStreamUrl() {
        const { PORT } = this.STREAM_CONFIG;
        return `${window.location.protocol}//${window.location.hostname}:${PORT}/stream`;
    }
    
    setupCameraStream() {
        const streamImg = document.getElementById('cameraStream');
        if (!streamImg) return;
        
        // Инициализация состояния стрима (Single Responsibility)
        this.initStreamState();
        
        // Установка обработчиков (Single Responsibility)
        this.setupStreamHandlers(streamImg);
        
        // Первоначальная загрузка
        streamImg.src = this.getStreamUrl();
    }
    
    // Single Responsibility: Только инициализация состояния
    initStreamState() {
        const { MAX_RECONNECT_ATTEMPTS, BASE_RECONNECT_DELAY } = this.STREAM_CONFIG;
        this.streamState = {
            isConnected: false,
            reconnectAttempts: 0,
            maxReconnectAttempts: MAX_RECONNECT_ATTEMPTS,
            reconnectDelay: BASE_RECONNECT_DELAY,
            reconnectTimeout: null,
            lastErrorTime: 0
        };
    }
    
    // Single Responsibility: Только установка обработчиков
    setupStreamHandlers(streamImg) {
        streamImg.onload = () => this.handleStreamConnect();
        streamImg.onerror = () => this.handleStreamError();
    }
    
    // Single Responsibility: Обработка успешного подключения
    handleStreamConnect() {
        if (!this.streamState.isConnected) {
            Logger.info('Видео поток подключен');
            this.streamState.isConnected = true;
            this.resetReconnectState();
            this.toggleStreamFallback(false);
        }
    }
    
    // Single Responsibility: Сброс состояния переподключения (DRY)
    resetReconnectState() {
        this.streamState.reconnectAttempts = 0;
        // Очищаем таймаут если есть
        if (this.streamState.reconnectTimeout) {
            clearTimeout(this.streamState.reconnectTimeout);
            this.streamState.reconnectTimeout = null;
        }
    }
    
    // Single Responsibility: Обработка ошибки стрима
    handleStreamError() {
        const now = Date.now();
        const { ERROR_DEBOUNCE } = this.STREAM_CONFIG;
        
        // KISS: Простая защита от множественных вызовов
        if (now - this.streamState.lastErrorTime < ERROR_DEBOUNCE) {
            return;
        }
        this.streamState.lastErrorTime = now;
        
        Logger.error('Ошибка загрузки видео потока');
        this.streamState.isConnected = false;
        this.toggleStreamFallback(true);
        this.attemptStreamReconnect();
    }
    
    // Single Responsibility: Только логика переподключения
    attemptStreamReconnect() {
        // Проверка лимита попыток
        if (this.streamState.reconnectAttempts >= this.streamState.maxReconnectAttempts) {
            Logger.error('Превышен лимит попыток переподключения стрима');
            this.updateFallbackMessage('Не удалось восстановить видео поток. Обновите страницу.');
            return;
        }
        
        this.streamState.reconnectAttempts++;
        // Экспоненциальная задержка с ограничением максимума (KISS)
        const calculatedDelay = this.streamState.reconnectDelay * this.streamState.reconnectAttempts;
        const delay = Math.min(calculatedDelay, this.STREAM_CONFIG.MAX_RECONNECT_DELAY);
        
        Logger.info(`Попытка переподключения ${this.streamState.reconnectAttempts}/${this.streamState.maxReconnectAttempts} через ${delay}ms`);
        this.updateFallbackMessage(`Переподключение... (попытка ${this.streamState.reconnectAttempts}/${this.streamState.maxReconnectAttempts})`);
        
        this.streamState.reconnectTimeout = setTimeout(() => this.reconnectStream(), delay);
    }
    
    // Single Responsibility: Только переподключение
    reconnectStream() {
        const streamImg = document.getElementById('cameraStream');
        if (streamImg) {
            // Добавляем timestamp для обхода кэша браузера
            streamImg.src = `${this.getStreamUrl()}?_cb=${Date.now()}`;
        }
    }
    
    // DRY: Единый метод для показа/скрытия fallback
    toggleStreamFallback(show) {
        const fallbackOverlay = document.getElementById('streamFallback');
        if (fallbackOverlay) {
            fallbackOverlay.classList.toggle('hidden', !show);
        }
    }
    
    // Single Responsibility: Только обновление сообщения
    updateFallbackMessage(message) {
        const messageEl = document.getElementById('streamFallbackMessage');
        if (messageEl) {
            messageEl.textContent = message;
        }
    }
    
    setupEventListeners() {
        // Полноэкранный режим
        const fullscreenBtn = document.getElementById('fullscreenBtn');
        if (fullscreenBtn) {
            fullscreenBtn.addEventListener('click', () => this.toggleFullscreen());
        }
        
        // Общие кнопки
        this.setupCommonButtons();
        
        // Настройки
        this.setupSettingsModal();
    }
    
    setupCommonButtons() {
        // Мобильные кнопки настроек и справки
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
        
        // Настройка модального окна справки (DRY - один метод для всех модальных окон)
        const helpModal = document.getElementById('helpModal');
        const helpCloseBtn = helpModal?.querySelector('.help-close');
        
        if (helpCloseBtn) {
            helpCloseBtn.addEventListener('click', () => this.closeHelp());
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
        
        // Чекбоксы настроек обновлений
        const autoUpdateCheck = document.getElementById('autoUpdate');
        const dontOfferCheck = document.getElementById('dontOfferUpdates');
        const channelSelect = document.getElementById('updateChannel');
        
        if (autoUpdateCheck) {
            autoUpdateCheck.addEventListener('change', () => this.saveUpdateSettings());
        }
        
        if (dontOfferCheck) {
            dontOfferCheck.addEventListener('change', () => this.saveUpdateSettings());
        }
        
        if (channelSelect) {
            channelSelect.addEventListener('change', () => {
                this.updateChannelDescription();
                this.saveUpdateSettings();
            });
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
    
    closeHelp() {
        const modal = document.getElementById('helpModal');
        if (modal) {
            modal.classList.add('hidden');
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
                    const confirmed = await ModalDialog.showConfirm(message, 'ℹ️ Доступно обновление');
                    if (confirmed) {
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
            }
            
            // Загружаем настройки
            const settingsResponse = await fetch('/api/update/settings');
            if (settingsResponse.ok) {
                const settings = await settingsResponse.json();
                const autoUpdateCheck = document.getElementById('autoUpdate');
                const dontOfferCheck = document.getElementById('dontOfferUpdates');
                const channelSelect = document.getElementById('updateChannel');
                
                if (autoUpdateCheck) autoUpdateCheck.checked = settings.autoUpdate;
                if (dontOfferCheck) dontOfferCheck.checked = settings.dontOffer;
                if (channelSelect && settings.channel) {
                    channelSelect.value = settings.channel;
                    this.updateChannelDescription();
                }
            }
        } catch (error) {
            Logger.error('Ошибка загрузки информации об обновлениях:', error);
        }
    }
    
    updateChannelDescription() {
        const channelSelect = document.getElementById('updateChannel');
        const channelDescription = document.getElementById('channelDescription');
        
        if (!channelSelect || !channelDescription) return;
        
        const channel = channelSelect.value;
        if (channel === 'dev') {
            channelDescription.textContent = '⚠️ Development сборки - могут содержать баги. Только для тестирования!';
            channelDescription.style.color = 'rgba(255, 136, 0, 0.9)';
        } else {
            channelDescription.textContent = 'Стабильные релизы для production использования';
            channelDescription.style.color = 'rgba(0, 255, 136, 0.7)';
        }
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
        if (checkBtn) {
            checkBtn.disabled = true;
            checkBtn.textContent = 'Проверка...';
        }
        
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
            
            // Проверяем обновления на GitHub API напрямую с клиента (KISS - Simple)
            const githubApiUrl = `https://api.github.com/repos/${this.GITHUB_REPO}/releases/latest`;
            const githubResponse = await fetch(githubApiUrl, {
                headers: {
                    'Accept': 'application/vnd.github+json',
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
            
            // Находим все доступные типы роботов из assets
            const availableTypes = this.extractAvailableTypesFromAssets(releaseData.assets);
            
            // Берем первый доступный файл как базовый URL (для определения формата)
            let downloadUrl = '';
            if (releaseData.assets && Array.isArray(releaseData.assets) && releaseData.assets.length > 0) {
                // Ищем любой валидный файл прошивки для определения формата
                const binAsset = releaseData.assets.find(asset => 
                    this.isValidFirmwareAsset(asset) && asset.browser_download_url
                );
                
                if (binAsset && binAsset.browser_download_url) {
                    downloadUrl = binAsset.browser_download_url;
                }
            }
            
            // Сравниваем версии
            const hasUpdate = this.isVersionNewer(currentVersion, latestVersion);
            
            const updateAvailableDiv = document.getElementById('updateAvailable');
            
            if (hasUpdate) {
                document.getElementById('newVersion').textContent = latestVersion;
                document.getElementById('newReleaseName').textContent = releaseName;
                document.getElementById('releaseNotes').textContent = releaseNotes;
                
                // Сохраняем базовый URL и версию
                this.baseUpdateUrl = downloadUrl;
                this.updateVersion = latestVersion;
                
                // Сохраняем информацию о релизе (DRY - Don't Repeat Yourself)
                this.latestReleaseInfo = {
                    version: latestVersion,
                    releaseName: releaseName,
                    releaseNotes: releaseNotes,
                    downloadUrl: downloadUrl
                };
                
                const selectionDiv = document.getElementById('robotTypeSelection');
                const downloadBtn = document.getElementById('downloadUpdateBtn');
                
                if (availableTypes.length > 0) {
                    // Показываем выбор типа робота пользователю
                    this.showRobotTypeSelection(availableTypes);
                } else {
                    // Универсальный бинарник или файлы не найдены - скрываем выбор
                    if (selectionDiv) selectionDiv.classList.add('hidden');
                    this.updateDownloadUrl = downloadUrl;
                }
                
                // Включаем кнопку загрузки
                if (downloadBtn) {
                    downloadBtn.textContent = '⬇️ Скачать обновление';
                    downloadBtn.disabled = false;
                }
                
                if (updateAvailableDiv) updateAvailableDiv.classList.remove('hidden');
                
                Logger.info('Доступно обновление:', {
                    current: currentVersion,
                    latest: latestVersion,
                    downloadUrl: downloadUrl
                });
            } else {
                if (updateAvailableDiv) updateAvailableDiv.classList.add('hidden');
                await ModalDialog.showAlert('У вас установлена последняя версия прошивки!', 'ℹ️ Информация');
            }
        } catch (error) {
            Logger.error('Ошибка проверки обновлений:', error);
            await ModalDialog.showAlert('Ошибка при проверке обновлений: ' + error.message, '❌ Ошибка');
        } finally {
            if (checkBtn) {
                checkBtn.disabled = false;
                checkBtn.textContent = 'Проверить обновления';
            }
        }
    }
    
    // Вспомогательная функция для проверки, является ли asset валидным файлом прошивки
    isValidFirmwareAsset(asset, robotType = null) {
        if (!asset || !asset.name) return false;
        
        const name = asset.name.toLowerCase();
        
        // Проверяем что это .bin файл (но не .bin.sha256)
        // Файлы контрольных сумм имеют расширение .bin.sha256
        if (!name.endsWith('.bin') || name.endsWith('.bin.sha256')) {
            return false;
        }
        
        // Если указан тип робота, проверяем соответствие
        if (robotType) {
            return name.includes(`microbox-${robotType}`);
        }
        
        // Если тип не указан, просто проверяем что это microbox файл
        return name.startsWith('microbox-');
    }
    
    extractAvailableTypesFromAssets(assets) {
        // Проверяем все assets и находим реально доступные типы роботов
        if (!assets || !Array.isArray(assets)) return [];
        
        const availableTypes = [];
        
        // Проходим по всем возможным типам и проверяем, есть ли соответствующий файл
        for (const type of BaseRobotUI.ROBOT_TYPES) {
            const found = assets.some(asset => this.isValidFirmwareAsset(asset, type));
            
            if (found) {
                availableTypes.push(type);
            }
        }
        
        return availableTypes;
    }
    
    // Функция сравнения версий (Single Responsibility - только сравнение версий)
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
        
        // Сравниваем каждую часть версии (major.minor.patch)
        for (let i = 0; i < Math.max(currentParts.length, latestParts.length); i++) {
            const current = currentParts[i] || 0;
            const latest = latestParts[i] || 0;
            
            if (latest > current) return true;
            if (latest < current) return false;
        }
        
        return false;
    }
    
    showRobotTypeSelection(availableTypes) {
        const selectionDiv = document.getElementById('robotTypeSelection');
        const selectElement = document.getElementById('robotTypeSelect');
        
        if (!selectionDiv || !selectElement) return;
        
        // Очищаем и заполняем select только доступными типами
        selectElement.innerHTML = '<option value="">-- Выберите тип --</option>';
        
        availableTypes.forEach(type => {
            const option = document.createElement('option');
            option.value = type;
            
            switch(type) {
                case 'classic':
                    option.textContent = '🚗 МикроБокс Классик (управляемый робот)';
                    break;
                case 'liner':
                    option.textContent = '🛤️ МикроБокс Лайнер (автономный, следование по линии)';
                    break;
                case 'brain':
                    option.textContent = '🎮 МикроБокс Брейн (модуль управления PWM/PPM/SBUS)';
                    break;
                default:
                    option.textContent = type;
            }
            
            selectElement.appendChild(option);
        });
        
        selectionDiv.classList.remove('hidden');
    }
    
    constructDownloadUrl(robotType) {
        // Формируем URL: microbox-{type}-{version}-release.bin или microbox-{type}-{version}.bin
        // в зависимости от того, какой формат используется в базовом URL
        if (!this.baseUpdateUrl || !this.updateVersion) return this.baseUpdateUrl;
        
        // Определяем, используется ли суффикс -release в базовом URL
        const hasReleaseSuffix = this.baseUpdateUrl.includes('-release.bin');
        
        // Заменяем имя файла в URL
        const urlParts = this.baseUpdateUrl.split('/');
        const suffix = hasReleaseSuffix ? '-release.bin' : '.bin';
        urlParts[urlParts.length - 1] = `microbox-${robotType}-${this.updateVersion}${suffix}`;
        
        return urlParts.join('/');
    }
    
    async downloadUpdate() {
        // Если есть выбор типа робота - сначала проверяем что выбрано
        const selectionDiv = document.getElementById('robotTypeSelection');
        if (selectionDiv && !selectionDiv.classList.contains('hidden')) {
            const selectElement = document.getElementById('robotTypeSelect');
            const selectedType = selectElement?.value;
            
            if (!selectedType) {
                await ModalDialog.showAlert('Выберите тип устройства для обновления', '⚠️ Внимание');
                return;
            }
            
            // Формируем URL для выбранного типа
            this.updateDownloadUrl = this.constructDownloadUrl(selectedType);
            Logger.info(`Выбран тип ${selectedType}, URL: ${this.updateDownloadUrl}`);
        }
        
        if (!this.updateDownloadUrl) {
            await ModalDialog.showAlert('URL для скачивания не найден', '❌ Ошибка');
            return;
        }
        
        const confirmed = await ModalDialog.showConfirm('Начать обновление прошивки?\nУстройство перезагрузится после завершения загрузки.', '⚠️ Обновление прошивки');
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
                    // Устройство перезагружается в safe mode для OTA
                    document.getElementById('firmwareStatus').textContent = 'Устройство перезагружается в безопасном режиме...';
                    
                    // Ждем перезагрузки
                    await new Promise(resolve => setTimeout(resolve, 5000));
                    
                    document.getElementById('firmwareStatus').textContent = 'Ожидание переподключения...';
                    
                    // Пробуем переподключиться и начать опрос статуса
                    let reconnectAttempts = 0;
                    const maxReconnectAttempts = 30; // 30 попыток * 2 секунды = 60 секунд
                    
                    const checkConnection = setInterval(async () => {
                        reconnectAttempts++;
                        
                        try {
                            const statusResponse = await fetch('/api/update/status');
                            if (statusResponse.ok) {
                                clearInterval(checkConnection);
                                document.getElementById('firmwareStatus').textContent = 'Устройство подключено! Обновление в процессе...';
                                
                                // Теперь начинаем обычный опрос статуса
                                this.pollFirmwareStatus();
                            }
                        } catch (error) {
                            console.log('Reconnect attempt ' + reconnectAttempts);
                            const progress = 15 + (reconnectAttempts / maxReconnectAttempts * 5);
                            const progressFill = document.getElementById('firmwareProgressFill');
                            const progressText = document.getElementById('firmwareProgressText');
                            if (progressFill) progressFill.style.width = progress + '%';
                            if (progressText) progressText.textContent = Math.round(progress) + '%';
                            
                            if (reconnectAttempts >= maxReconnectAttempts) {
                                clearInterval(checkConnection);
                                await ModalDialog.showAlert('Не удалось переподключиться к устройству после перезагрузки', '❌ Ошибка');
                                this.hideFirmwareUpdateOverlay();
                            }
                        }
                    }, 2000);
                    
                } else if (data.updating) {
                    // Обновление началось без перезагрузки - начинаем опрос статуса
                    document.getElementById('firmwareStatus').textContent = 'Загрузка прошивки...';
                    this.pollFirmwareStatus();
                }
            } else {
                await ModalDialog.showAlert('Ошибка запуска обновления', '❌ Ошибка');
                this.hideFirmwareUpdateOverlay();
            }
        } catch (error) {
            Logger.error('Ошибка загрузки обновления:', error);
            await ModalDialog.showAlert('Ошибка подключения к устройству', '❌ Ошибка');
            this.hideFirmwareUpdateOverlay();
        }
    }
    
    pollFirmwareStatus() {
        // Опрашиваем статус обновления каждую секунду
        const pollInterval = setInterval(async () => {
            try {
                const response = await fetch('/api/update/status');
                if (!response.ok) {
                    Logger.warn('Не удалось получить статус обновления');
                    return;
                }
                
                const status = await response.json();
                
                // Обновляем UI
                const statusEl = document.getElementById('firmwareStatus');
                const progressFill = document.getElementById('firmwareProgressFill');
                const progressText = document.getElementById('firmwareProgressText');
                
                if (statusEl) statusEl.textContent = status.status || 'Обновление...';
                if (progressFill) progressFill.style.width = (status.progress || 0) + '%';
                if (progressText) progressText.textContent = (status.progress || 0) + '%';
                
                // Проверяем состояние
                if (status.state === 3) { // SUCCESS
                    clearInterval(pollInterval);
                    if (statusEl) statusEl.textContent = 'Обновление завершено! Перезагрузка...';
                    if (progressFill) progressFill.style.width = '100%';
                    if (progressText) progressText.textContent = '100%';
                    
                    setTimeout(() => {
                        this.hideFirmwareUpdateOverlay();
                        location.reload();
                    }, 3000);
                } else if (status.state === 4) { // FAILED
                    clearInterval(pollInterval);
                    if (statusEl) statusEl.textContent = 'Ошибка обновления: ' + (status.status || 'Неизвестная ошибка');
                    await ModalDialog.showAlert('Ошибка обновления прошивки', '❌ Ошибка');
                    setTimeout(() => {
                        this.hideFirmwareUpdateOverlay();
                    }, 3000);
                }
            } catch (error) {
                Logger.error('Ошибка опроса статуса:', error);
                // Не прерываем опрос при единичных ошибках
            }
        }, 1000);
        
        // Таймаут на случай зависания (2 минуты)
        setTimeout(() => {
            clearInterval(pollInterval);
        }, 120000);
    }
    
    async saveUpdateSettings() {
        const autoUpdate = document.getElementById('autoUpdate')?.checked || false;
        const dontOffer = document.getElementById('dontOfferUpdates')?.checked || false;
        const channel = document.getElementById('updateChannel')?.value || 'stable';
        
        try {
            await fetch('/api/update/settings', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ autoUpdate, dontOffer, channel })
            });
            Logger.log(`Настройки обновлений сохранены (канал: ${channel})`);
        } catch (error) {
            Logger.error('Ошибка сохранения настроек обновлений:', error);
        }
    }
    
    showFirmwareUpdateOverlay() {
        const overlay = document.getElementById('firmwareUpdateOverlay');
        if (overlay) overlay.classList.remove('hidden');
        
        // Заполняем информацию о прошивке если есть
        if (this.latestReleaseInfo) {
            const versionEl = document.getElementById('firmwareVersion');
            const releaseNameEl = document.getElementById('firmwareReleaseName');
            const notesEl = document.getElementById('firmwareReleaseNotes');
            
            if (versionEl) versionEl.textContent = `Версия: ${this.latestReleaseInfo.version}`;
            if (releaseNameEl) releaseNameEl.textContent = this.latestReleaseInfo.releaseName || '';
            if (notesEl) {
                // Обрезаем длинные заметки для оверлея
                const notes = this.latestReleaseInfo.releaseNotes || '';
                const shortNotes = notes.length > 200 ? notes.substring(0, 200) + '...' : notes;
                notesEl.textContent = shortNotes;
            }
        }
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
        
        // Маппинг эффектов для API (DRY)
        this.effectMap = { normal: 0, police: 1, fire: 2, ambulance: 3, terminator: 4 };
        
        // PWM константы для моторов (KISS)
        this.PWM_NEUTRAL = 1500;
        this.PWM_FORWARD = 2000;
        this.PWM_BACKWARD = 1000;
        this.PWM_LEFT = 1000;
        this.PWM_RIGHT = 2000;
        
        // T-800 overlay
        this.t800Interval = null;
        this.t800StartTime = null;
    }
    
    setupEventListeners() {
        super.setupEventListeners();
        // PC контролы удалены - используется только мобильный интерфейс
    }
    
    setupSaveButtons() {
        // Обработчики для кнопок настроек
        const testMotorBtn = document.getElementById('testMotorBtn');
        if (testMotorBtn) {
            testMotorBtn.addEventListener('click', () => this.testMotor());
        }
        
        const saveSettingsBtn = document.getElementById('saveSettings');
        if (saveSettingsBtn) {
            saveSettingsBtn.addEventListener('click', () => this.saveSettings());
        }
        
        const saveMotorBtn = document.getElementById('saveMotorConfig');
        if (saveMotorBtn) {
            saveMotorBtn.addEventListener('click', () => this.saveMotorSettings());
        }
        
        const saveCameraBtn = document.getElementById('saveCameraConfig');
        if (saveCameraBtn) {
            saveCameraBtn.addEventListener('click', () => this.saveCameraSettings());
        }
        
        const saveWiFiBtn = document.getElementById('saveWiFi');
        if (saveWiFiBtn) {
            saveWiFiBtn.addEventListener('click', () => this.saveWiFiSettings());
        }
        
        const restartBtn = document.getElementById('restartDevice');
        if (restartBtn) {
            restartBtn.addEventListener('click', () => this.restartDevice());
        }
    }
    
    async setEffectMode(mode) {
        this.effectMode = mode;
        
        const effectId = this.effectMap[mode] || 0;
        
        try {
            await fetch(`/cmd?effect=${effectId}`);
        } catch (error) {
            Logger.error('Ошибка установки эффекта:', error);
        }
    }
    
    async testMotor() {
        const motorSide = document.querySelector('input[name="testMotor"]:checked')?.value || 'left';
        Logger.info(`Тестирование ${motorSide} мотора`);
        
        try {
            // Тест: полный газ вперёд + руль в сторону выбранного мотора
            const throttle = this.PWM_FORWARD;
            const steering = motorSide === 'left' ? this.PWM_LEFT : this.PWM_RIGHT;
            
            await fetch(`/cmd?throttle=${throttle}&steering=${steering}`);
            
            // Через 1 секунду останавливаем (с обработкой ошибок)
            setTimeout(async () => {
                try {
                    await fetch(`/cmd?throttle=${this.PWM_NEUTRAL}&steering=${this.PWM_NEUTRAL}`);
                } catch (error) {
                    Logger.error('Ошибка остановки мотора:', error);
                }
            }, 1000);
        } catch (error) {
            Logger.error('Ошибка тестирования мотора:', error);
        }
    }
    
    async saveSettings() {
        // Собираем настройки эффектов и чувствительности
        const effectMode = document.querySelector('input[name="effectMode"]:checked')?.value || 'normal';
        const effectId = this.effectMap[effectMode] || 0;
        
        const settings = {
            speedSensitivity: parseInt(document.getElementById('speedSensitivity')?.value) || 80,
            turnSensitivity: parseInt(document.getElementById('turnSensitivity')?.value) || 70,
            effectMode: effectId
        };
        
        // Применяем локально
        this.speedSensitivity = settings.speedSensitivity;
        this.turnSensitivity = settings.turnSensitivity;
        
        // Сохраняем в localStorage
        localStorage.setItem('robotSettings', JSON.stringify({
            speedSensitivity: settings.speedSensitivity,
            turnSensitivity: settings.turnSensitivity,
            effectMode: effectMode
        }));
        
        try {
            // Сохраняем настройки через API
            const response = await fetch('/api/settings/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(settings)
            });
            
            if (response.ok) {
                await this.setEffectMode(effectMode);
                Logger.info('Настройки сохранены');
            } else {
                Logger.error('Ошибка сохранения настроек');
            }
        } catch (error) {
            Logger.error('Ошибка сохранения настроек:', error);
        }
    }
    
    async saveMotorSettings() {
        // Собираем только настройки моторов (частичное обновление)
        const settings = {
            swapLeftRight: document.getElementById('motorSwapLeftRight')?.checked || false,
            invertLeft: document.getElementById('motorInvertLeft')?.checked || false,
            invertRight: document.getElementById('motorInvertRight')?.checked || false,
            invertThrottle: document.getElementById('invertThrottleStick')?.checked || false,
            invertSteering: document.getElementById('invertSteeringStick')?.checked || false
        };
        
        try {
            const response = await fetch('/api/settings/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(settings)
            });
            
            if (response.ok) {
                const result = await response.json();
                Logger.info('Настройки моторов сохранены и применены');
                // Моторы применяются сразу, needRestart не ожидается
            } else {
                Logger.error('Ошибка сохранения настроек моторов');
            }
        } catch (error) {
            Logger.error('Ошибка сохранения настроек моторов:', error);
        }
    }
    
    async saveCameraSettings() {
        // Собираем настройки камеры
        const settings = {
            hMirror: document.getElementById('cameraHMirror')?.checked || false,
            vFlip: document.getElementById('cameraVFlip')?.checked || false
        };
        
        try {
            // Сохраняем настройки
            const saveResponse = await fetch('/api/settings/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(settings)
            });
            
            if (saveResponse.ok) {
                Logger.info('Настройки камеры сохранены');
                
                // Применяем настройки без перезагрузки
                const applyResponse = await fetch('/api/camera/apply', {
                    method: 'POST'
                });
                
                if (applyResponse.ok) {
                    Logger.info('Настройки камеры применены. Изменения видны на экране.');
                } else {
                    Logger.error('Ошибка применения настроек камеры');
                }
            } else {
                Logger.error('Ошибка сохранения настроек камеры');
            }
        } catch (error) {
            Logger.error('Ошибка сохранения настроек камеры:', error);
        }
    }
    
    async saveWiFiSettings() {
        const mode = document.getElementById('wifiMode')?.value;
        const ssid = document.getElementById('wifiSSID')?.value;
        const password = document.getElementById('wifiPassword')?.value;
        
        if (!ssid) {
            Logger.error('SSID не может быть пустым');
            return;
        }
        
        try {
            const response = await fetch('/api/settings/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ ssid, password, mode })
            });
            
            if (response.ok) {
                const result = await response.json();
                if (result.needRestart) {
                    Logger.info('WiFi настройки сохранены. Требуется перезагрузка.');
                    // Можно предложить перезагрузку
                    const shouldRestart = await ModalDialog.showConfirm('Для применения WiFi настроек требуется перезагрузка. Перезагрузить сейчас?', '🔄 Требуется перезагрузка');
                    if (shouldRestart) {
                        this.restartDevice();
                    }
                } else {
                    Logger.info('WiFi настройки сохранены');
                }
            } else {
                Logger.error('Ошибка сохранения WiFi настроек');
            }
        } catch (error) {
            Logger.error('Ошибка сохранения WiFi настроек:', error);
        }
    }
    
    async restartDevice() {
        const confirmed = await ModalDialog.showConfirm('Вы уверены, что хотите перезагрузить устройство?', '🔄 Перезагрузка устройства');
        if (!confirmed) {
            return;
        }
        
        try {
            await fetch('/api/restart', { method: 'POST' });
            Logger.info('Устройство перезагружается...');
        } catch (error) {
            Logger.error('Ошибка перезагрузки устройства:', error);
        }
    }
    
    async loadSettings() {
        // Загружаем все настройки с сервера (WiFi + моторы + стики)
        try {
            const response = await fetch('/api/settings/get');
            if (response.ok) {
                const data = await response.json();
                
                // Применяем WiFi настройки к UI
                if (data.wifi) {
                    const ssidEl = document.getElementById('wifiSSID');
                    const modeEl = document.getElementById('wifiMode');
                    if (ssidEl) ssidEl.value = data.wifi.ssid || '';
                    if (modeEl) modeEl.value = data.wifi.mode || 'CLIENT';
                }
                
                // Применяем настройки моторов к UI
                const setChecked = (id, value) => {
                    const el = document.getElementById(id);
                    if (el) el.checked = value || false;
                };
                
                if (data.motors) {
                    setChecked('motorSwapLeftRight', data.motors.swapLeftRight);
                    setChecked('motorInvertLeft', data.motors.invertLeft);
                    setChecked('motorInvertRight', data.motors.invertRight);
                }
                
                if (data.sticks) {
                    setChecked('invertThrottleStick', data.sticks.invertThrottle);
                    setChecked('invertSteeringStick', data.sticks.invertSteering);
                }
                
                // Применяем настройки камеры к UI
                if (data.camera) {
                    setChecked('cameraHMirror', data.camera.hMirror);
                    setChecked('cameraVFlip', data.camera.vFlip);
                }
            }
        } catch (error) {
            Logger.debug('Не удалось загрузить настройки:', error);
        }
        
        // Загружаем чувствительность из localStorage (локальные настройки UI)
        try {
            const saved = localStorage.getItem('robotSettings');
            if (saved) {
                const settings = JSON.parse(saved);
                this.speedSensitivity = settings.speedSensitivity || 80;
                this.turnSensitivity = settings.turnSensitivity || 70;
                
                // Устанавливаем значения в UI
                const speedEl = document.getElementById('speedSensitivity');
                const turnEl = document.getElementById('turnSensitivity');
                if (speedEl) speedEl.value = this.speedSensitivity;
                if (turnEl) turnEl.value = this.turnSensitivity;
            }
        } catch (error) {
            Logger.debug('Не удалось загрузить настройки чувствительности:', error);
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
            if (!isDragging) return;  // Защита от повторного вызова
            
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
            if (isDragging && touchId === null) {  // Только для мыши (touchId = null)
                e.preventDefault();
                handleMove(e.clientX, e.clientY);
            }
        });
        
        document.addEventListener('mouseup', () => {
            if (isDragging && touchId === null) {  // Только для мыши
                handleEnd();
            }
        });
        
        // Touch события
        element.addEventListener('touchstart', (e) => {
            if (isDragging) return; // Предотвращаем повторную активацию
            
            e.preventDefault();
            const touch = e.touches[0];
            handleStart(touch.clientX, touch.clientY, touch.identifier);
        }, { passive: false });
        
        element.addEventListener('touchmove', (e) => {
            if (!isDragging || touchId === null) return;
            
            // Ищем наш touch среди всех активных touches
            for (let i = 0; i < e.touches.length; i++) {
                if (e.touches[i].identifier === touchId) {
                    e.preventDefault();
                    const touch = e.touches[i];
                    handleMove(touch.clientX, touch.clientY);
                    return;
                }
            }
        }, { passive: false });
        
        element.addEventListener('touchend', (e) => {
            if (!isDragging || touchId === null) return;
            
            // Проверяем завершенные touches
            for (let i = 0; i < e.changedTouches.length; i++) {
                if (e.changedTouches[i].identifier === touchId) {
                    e.preventDefault();
                    handleEnd();
                    return;
                }
            }
        }, { passive: false });
        
        // Дополнительная защита: touchcancel
        element.addEventListener('touchcancel', (e) => {
            if (!isDragging || touchId === null) return;
            
            for (let i = 0; i < e.changedTouches.length; i++) {
                if (e.changedTouches[i].identifier === touchId) {
                    Logger.warn(`Touch ${touchId} cancelled for ${side} joystick`);
                    handleEnd();
                    return;
                }
            }
        }, { passive: false });
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
