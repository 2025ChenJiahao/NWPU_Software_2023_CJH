from flask import Flask, render_template, request, jsonify
import subprocess
import tempfile
import os

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/compile', methods=['POST'])
def compile_code():
    code = request.json.get('code')
    
    # Create a temporary file for the input
    with tempfile.NamedTemporaryFile(suffix='.txt', delete=False) as f:
        f.write(code.encode('utf-8'))
        temp_file = f.name
    
    try:
        # Run lexical analysis
        lex_result = subprocess.run(
            ['./bin/sonnx-compiler', '-t', temp_file],
            capture_output=True,
            text=True,
            cwd=os.path.dirname(os.path.abspath(__file__))
        )
        
        # Run syntax analysis
        syntax_result = subprocess.run(
            ['./bin/sonnx-compiler', '-p', temp_file],
            capture_output=True,
            text=True,
            cwd=os.path.dirname(os.path.abspath(__file__))
        )
        
        # Run semantic analysis
        semantic_result = subprocess.run(
            ['./bin/sonnx-compiler', '-a', temp_file],
            capture_output=True,
            text=True,
            cwd=os.path.dirname(os.path.abspath(__file__))
        )
        
        # Run code generation
        codegen_result = subprocess.run(
            ['./bin/sonnx-compiler', '-c', temp_file],
            capture_output=True,
            text=True,
            cwd=os.path.dirname(os.path.abspath(__file__))
        )
        
        # Run full compilation
        full_result = subprocess.run(
            ['./bin/sonnx-compiler', temp_file],
            capture_output=True,
            text=True,
            cwd=os.path.dirname(os.path.abspath(__file__))
        )
        
        return jsonify({
            'lexical': lex_result.stdout or lex_result.stderr,
            'syntax': syntax_result.stdout or syntax_result.stderr,
            'semantic': semantic_result.stdout or semantic_result.stderr,
            'codegen': codegen_result.stdout or codegen_result.stderr,
            'full': full_result.stdout or full_result.stderr
        })
    finally:
        # Clean up temporary file
        if os.path.exists(temp_file):
            os.unlink(temp_file)

if __name__ == '__main__':
    app.run(debug=True, host='localhost', port=5000)