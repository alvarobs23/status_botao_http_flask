from flask import Flask, render_template, request
from flask_socketio import SocketIO

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/update', methods=['GET'])
def update():
    # Espera receber ?btn=<0|1>&temp=<valor>
    btn  = request.args.get('btn', type=int)
    temp = request.args.get('temp', type=float)
    print(f"Recebido: btn={btn}, temp={temp:.2f}°C")
    # Emite para todos os clientes conectados
    socketio.emit('status', {
        'btn':  btn, 
        'temp': f"{temp:.2f}"
    })
    return 'OK', 200

# (Opcional) mantém suas rotas existentes de CLICK/SOLTO
@app.route('/CLICK', methods=['GET','POST'])
def click():
    socketio.emit('command', {'action': 'click'})
    return 'Click command sent', 200

@app.route('/SOLTO', methods=['GET','POST'])
def solto():
    socketio.emit('command', {'action': 'solto'})
    return 'solto command sent', 200

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000)
