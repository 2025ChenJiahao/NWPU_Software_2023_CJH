from database.db import get_db


def create_user(username, email, password_hash, platform):
    db = get_db()
    db.execute(
        'INSERT INTO users (username, email, password_hash, platform) VALUES (?, ?, ?, ?)',
        (username, email, password_hash, platform)
    )
    db.commit()


def get_user_by_username(username):
    db = get_db()
    return db.execute('SELECT * FROM users WHERE username = ?', (username,)).fetchone()


def get_user_by_email(email):
    db = get_db()
    return db.execute('SELECT * FROM users WHERE email = ?', (email,)).fetchone()


def username_exists(username):
    db = get_db()
    row = db.execute('SELECT 1 FROM users WHERE username = ?', (username,)).fetchone()
    return row is not None


def email_exists(email):
    db = get_db()
    row = db.execute('SELECT 1 FROM users WHERE email = ?', (email,)).fetchone()
    return row is not None


def update_last_login(user_id):
    db = get_db()
    db.execute(
        "UPDATE users SET last_login = datetime('now', 'localtime') WHERE id = ?",
        (user_id,)
    )
    db.commit()
