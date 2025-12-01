from flask import Flask, render_template
import random
import datetime

app = Flask(__name__)

# --- ROUTE HOME ---
@app.route('/')
def home():
    data = {'title': 'KING RON IOT'}
    return render_template('index.html', data=data)

# --- ROUTE MONITORING (GRAFIK) ---
@app.route('/monitoring')
def monitoring_page():
    # Data Dummy buat Grafik
    live_data = {
        'suhu': 29,       
        'kelembapan': 75, 
        'soil': 60        
    }
    jam_labels = [f"{i:02d}:00" for i in range(24)] 
    data_suhu = [random.randint(28, 33) for _ in range(24)]
    data_lembap = [random.randint(60, 80) for _ in range(24)]

    return render_template('monitoring.html', 
                           live=live_data, 
                           labels=jam_labels, 
                           suhu=data_suhu, 
                           lembap=data_lembap)

# --- ROUTE DATA LOG (DUMMY DATA - BIAR GAK CRASH) ---
@app.route('/datalog')
def datalog_page():
    # Kita pura-pura punya data log biar halaman bisa dibuka
    logs = [
        {'waktu': '10:05:00', 'suhu': 32, 'kelembapan': 70, 'soil': 50},
        {'waktu': '10:00:00', 'suhu': 29, 'kelembapan': 75, 'soil': 60},
        {'waktu': '09:55:00', 'suhu': 28, 'kelembapan': 76, 'soil': 65},
        {'waktu': '09:50:00', 'suhu': 30, 'kelembapan': 72, 'soil': 62},
    ]
    return render_template('datalog.html', logs=logs)

if __name__ == '__main__':
    # Host 0.0.0.0 biar bisa diakses HP (syarat: satu wifi)
    app.run(debug=True, host='0.0.0.0')
    # app.run(debug=True)