from flask import Flask, request, jsonify
import logging

app = Flask(__name__)

log = logging.getLogger('werkzeug')
log.disabled = True

# Variables to store player usernames and scores
player1 = None
player2 = None
score1 = 0
score2 = 0
last_replaced = 0  # Tracks which player was set first (0 for player1, 1 for player2)

@app.route("/", methods=["POST"])
def receive_data():
    global player1, player2, score1, score2, last_replaced
    data = request.get_json()

    if not data or "iotMessage" not in data:
        return jsonify({"error": "Invalid request"}), 400

    iot_message = data["iotMessage"]  

    if "username" not in iot_message:
        return jsonify({"error": "Invalid request"}), 400

    username = iot_message["username"]

    if len(iot_message) == 1:
        if not player1:
            player1 = username
            last_replaced = 0  # Player1 was set first
        elif not player2 and username != player1:
            player2 = username
            last_replaced = 1  # Player2 was set last
        elif player1 and player2 and username not in [player1, player2]:
            if last_replaced == 0:
                player1 = username
                score1 = 0  
                last_replaced = 1  
            else:
                player2 = username
                score2 = 0  
                last_replaced = 0 
        # Ensure alphabetical ordering
        if player1 and player2 and player1 > player2:
            player1, player2 = player2, player1
            score1, score2 = score2, score1  

        print(f"Updated Players: Player1 = {player1}, Player2 = {player2}")

    # Handle game state updates (assigning scores)
    elif "my_score" in iot_message and "opponent_score" in iot_message:
        my_score = iot_message["my_score"]
        opponent_score = iot_message["opponent_score"]

        if username == player1:
            score1 = my_score
            score2 = opponent_score
        elif username == player2:
            score2 = my_score
            score1 = opponent_score

        print(f"Updated Scores: {player1} = {score1}, {player2} = {score2}")

    return jsonify({"status": "Received"}), 200

@app.route("/get_status", methods=["GET"])
def get_status():
    return jsonify({
        "player1": player1,
        "player2": player2,
        "score1": score1,
        "score2": score2
    })

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
