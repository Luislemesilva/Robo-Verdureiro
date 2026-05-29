from flask import Flask, request, jsonify
import joblib
import pandas as pd
import os

app = Flask(__name__)

knn = joblib.load("modelo_knn.pkl")
scaler = joblib.load("scaler.pkl")

@app.route("/")
def raiz():
    return jsonify({"mensagem": "Robô Verdureiro no ar!"})

@app.route("/health", methods=["GET"])
def health():
    return jsonify({
        "status": "ok",
        "modelo": "KNN carregado"
    })

@app.route("/prever", methods=["POST"])
def prever():

    try:

        dados = request.get_json(force=True)

        print("JSON recebido:")
        print(dados)

        campos = [
            "precipitacao",
            "temperatura",
            "orvalho",
            "umidade",
            "vento"
        ]

        for campo in campos:
            if campo not in dados:
                return jsonify({
                    "erro": f"Campo ausente: {campo}"
                }), 400

        entrada = pd.DataFrame([{
            "Precipitação Total (mm)": dados["precipitacao"],
            "Temperatura do Ar (°C)": dados["temperatura"],
            "Temperatura do Orvalho (°C)": dados["orvalho"],
            "Umidade Total": dados["umidade"],
            "Velocidade do Vento": dados["vento"]
        }])

        entrada_scaled = scaler.transform(entrada)

        predicao = knn.predict(entrada_scaled)[0]

        resultado = "SIM" if predicao == 1 else "NAO"

        print(f"Resultado: {resultado}")

        return jsonify({
            "irrigar": resultado
        })

    except Exception as e:

        print("ERRO:")
        print(str(e))

        return jsonify({
            "erro": str(e)
        }), 500
