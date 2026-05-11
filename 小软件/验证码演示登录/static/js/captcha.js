// 验证码刷新功能
document.addEventListener('DOMContentLoaded', function() {
    const captchaImg = document.getElementById('captchaImg');
    if (captchaImg) {
        captchaImg.addEventListener('click', function() {
            this.src = '/api/captcha?t=' + Date.now();
        });
    }
});
