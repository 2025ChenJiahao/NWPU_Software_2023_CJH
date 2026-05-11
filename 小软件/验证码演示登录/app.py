from flask import Flask
import config
from database.db import init_app


def create_app():
    app = Flask(__name__)
    app.config.from_object(config)

    # 初始化数据库
    init_app(app)

    # 注册蓝图
    from auth import bp as auth_bp
    app.register_blueprint(auth_bp)

    return app


if __name__ == '__main__':
    app = create_app()
    app.run(debug=True, host='0.0.0.0', port=5000)
