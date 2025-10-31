function openTab(evt, tabName) {
  var i, tabcontent, tablinks;
  tabcontent = document.getElementsByClassName("tabcontent");
  for (i = 0; i < tabcontent.length; i++) {
    tabcontent[i].style.display = "none";
  }
  tablinks = document.getElementsByClassName("tablinks");
  for (i = 0; i < tablinks.length; i++) {
    tablinks[i].className = tablinks[i].className.replace(" active", "");
  }
  document.getElementById(tabName).style.display = "block";
  evt.currentTarget.className += " active";
}

// Показать первую вкладку по умолчанию
document.addEventListener("DOMContentLoaded", function () {
  document.querySelector('.tablinks:not(.hidden)').click();

  // Загружаем информацию об устройстве и отображаем её
  fetch("device_info")
    .then((response) => response.json())
    .then((data) => {
      const deviceInfoTable = document
        .getElementById("deviceInfoTable")
        .getElementsByTagName("tbody")[0];
      const firmwareVersion = document.getElementById("firmwareVersion");
      const targetVersion = document.getElementById("targetVersion");
      const targetVersionCode = document.getElementById("targetVersionCode");

      // Записываем данные о диапазонах
      if (data.RADIO_1_2G_ENABLED) {
        deviceInfoTable.insertRow().innerHTML = `
                  <td>1.2ГГц</td>
                  <td>${data.MAX_CHANNELS_1_2G}</td>
                  <td>${data.MIN_1200_FREQ} - ${data.MAX_1200_FREQ} МГц</td>
                  <td>${data.RSSI_BAND_1_2_MIN} - ${data.RSSI_BAND_1_2_MAX}</td>
              `;
      }

      if (data.RADIO_2_4G_ENABLED) {
        deviceInfoTable.insertRow().innerHTML = `
                  <td>2.4ГГц</td>
                  <td>${data.MAX_CHANNELS_2_4G}</td>
                  <td>${data.MIN_2400_FREQ} - ${data.MAX_2400_FREQ} МГц</td>
                  <td>${data.RSSI_BAND_2_4_MIN} - ${data.RSSI_BAND_2_4_MAX}</td>
              `;
      }

      if (data.RADIO_5_8G_ENABLED) {
        deviceInfoTable.insertRow().innerHTML = `
                  <td>5.8ГГц</td>
                  <td>${data.MAX_CHANNELS_5_8G}</td>
                  <td>${data.MIN_5800_FREQ} - ${data.MAX_5800_FREQ} МГц</td>
                  <td>${data.RSSI_BAND_5_8_MIN} - ${data.RSSI_BAND_5_8_MAX}</td>
              `;
      }

      firmwareVersion.textContent = data.FIRMWARE_VERSION;
      targetVersion.textContent = data.TARGET_VERSION;
      targetVersionCode.textContent = "-" + data.TARGET_VERSION;

      // Initialize form values
      initializeSettings(data);
    })
    .catch((error) =>
      console.error("Ошибка загрузки информации об устройстве:", error)
    );
});

document
  .getElementById("updateForm")
  .addEventListener("submit", function (event) {
    event.preventDefault();
    var form = event.target;
    var submitButton = document.getElementById("submitButton");
    var preloader = document.getElementById("preloader");
    var resultMessage = document.getElementById("resultMessage");
    var message = document.getElementById("message");
    resultMessage.style.display = "none";
    message.textContent = "";

    // Добавим проверку имени файла
    var fileInput = form.querySelector('input[type="file"]');
    var file = fileInput.files[0];

    submitButton.disabled = true;
    preloader.style.display = "flex";

    var xhr = new XMLHttpRequest();
    xhr.open("POST", form.action, true);
    xhr.setRequestHeader("X-FileSize", file.size);
    xhr.onload = function () {
      preloader.style.display = "none";
      resultMessage.style.display = "block";

      if (xhr.status === 200) {
        message.textContent = "Обновление успешно завершено!";
        resultMessage.className = "result-message";
      } else {
        message.textContent = "Произошла ошибка при загрузке!";
        resultMessage.className = "result-message error";
      }

      submitButton.disabled = false;
    };
    xhr.onerror = function () {
      preloader.style.display = "none";
      resultMessage.style.display = "block";
      message.textContent = "Произошла ошибка при загрузке!";
      resultMessage.className = "result-message error";
      submitButton.disabled = false;
    };

    var formData = new FormData(form);
    xhr.send(formData);
  });

let footerClickCount = 0;
document.getElementById("footer").addEventListener("click", function () {
  footerClickCount++;
  if (footerClickCount >= 10) {
    document.getElementById("settingsTabButton").classList.remove("hidden");
    document.getElementById("settingsTabButton").click();
  }
});

document
  .getElementById("settingsForm")
  .addEventListener("submit", function (event) {
    event.preventDefault();
    const formData = new FormData(event.target);

    fetch("/update_rssi", {
      method: "POST",
      body: formData,
    })
      .then((response) => response.json())
      .then((data) => {
        // Handle success
        console.log("Success:", data);
        location.reload();
      })
      .catch((error) => {
        console.error("Error:", error);
        var resultMessage = document.getElementById("resultMessage");
        var message = document.getElementById("message");
        resultMessage.style.display = "block";
        message.textContent = "Произошла ошибка при сохранении!";
      });
  });
function initializeSettings(data) {
  document.getElementById("RSSI_BAND_1_2_MIN").value = data.RSSI_BAND_1_2_MIN;
  document.getElementById("RSSI_BAND_1_2_MAX").value = data.RSSI_BAND_1_2_MAX;
  document.getElementById("RSSI_BAND_2_4_MIN").value = data.RSSI_BAND_2_4_MIN;
  document.getElementById("RSSI_BAND_2_4_MAX").value = data.RSSI_BAND_2_4_MAX;
  document.getElementById("RSSI_BAND_5_8_MIN").value = data.RSSI_BAND_5_8_MIN;
  document.getElementById("RSSI_BAND_5_8_MAX").value = data.RSSI_BAND_5_8_MAX;
}