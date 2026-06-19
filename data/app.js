document.addEventListener('DOMContentLoaded', () => {
    fetch('/api/config')
        .then(response => response.json())
        .then(data => {
            document.getElementById('config-box').innerText = `Device: ${data.device} (Threshold: ${data.threshold_cm}cm)`;
        });
});