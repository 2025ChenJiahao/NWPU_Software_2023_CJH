// 邮箱验证码发送功能
document.addEventListener('DOMContentLoaded', function() {
    const sendBtn = document.getElementById('sendCodeBtn');
    const emailInput = document.getElementById('emailInput');

    if (!sendBtn || !emailInput) return;

    let countdown = null;

    sendBtn.addEventListener('click', function() {
        const email = emailInput.value.trim();
        if (!email) {
            alert('请先输入邮箱');
            return;
        }

        if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)) {
            alert('请输入有效的邮箱地址');
            return;
        }

        sendBtn.disabled = true;
        sendBtn.textContent = '发送中...';

        fetch('/api/send-code', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ email: email })
        })
        .then(res => res.json())
        .then(data => {
            if (data.ok) {
                if (data.demo) {
                    // 演示模式：显示验证码
                    alert('演示模式 - 您的验证码是: ' + data.code);
                }
                startCountdown(60);
            } else {
                alert(data.error || '发送失败');
                sendBtn.disabled = false;
                sendBtn.textContent = '发送验证码';
            }
        })
        .catch(err => {
            alert('网络错误，请重试');
            sendBtn.disabled = false;
            sendBtn.textContent = '发送验证码';
        });
    });

    function startCountdown(seconds) {
        let remaining = seconds;
        sendBtn.textContent = remaining + '秒后重发';

        countdown = setInterval(function() {
            remaining--;
            if (remaining <= 0) {
                clearInterval(countdown);
                sendBtn.disabled = false;
                sendBtn.textContent = '发送验证码';
            } else {
                sendBtn.textContent = remaining + '秒后重发';
            }
        }, 1000);
    }
});
