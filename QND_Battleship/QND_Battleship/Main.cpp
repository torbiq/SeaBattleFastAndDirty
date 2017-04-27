#include <SFML\Graphics.hpp>
#include <SFML\Network.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <thread>

sf::TcpSocket serverSocket;
sf::TcpListener listener;

// Finished.
const int ROWS = 10;
const int COLS = 10;
const int MAX_BUFF = 200;
const int COUNT_BATTLESHIPS = 0;
const int COUNT_CRUISERS = 0;
const int COUNT_DESTROYERS = 0;
const int COUNT_SUBMARINES = 1;

const size_t buffSize = MAX_BUFF;

std::vector<sf::RectangleShape> _ownFieldSquaredRects;			// Own client's field squares.
std::vector<sf::RectangleShape> _enemyFieldSquaredRects;		// Enemy field squares.

int _squaresCount = 10;
int _pixelSizeSquare = 50;
int _pixelSizeSquaresOutline = 1;
int _pixelSizeBlockWithOutline = _pixelSizeSquare + _pixelSizeSquaresOutline;
int _squaredField = _pixelSizeBlockWithOutline * _squaresCount;
int _distanceBetweenFields = 10;
int _offsetXPlayerfield = _squaredField + _distanceBetweenFields;

const std::string CMD_PLACING_SHIPS = "placing";
const std::string CMD_FIGHTING = "fighting";
const std::string CMD_GAME_FINISHED = "finished";
const std::string CMD_WON = "won";
const std::string CMD_LOST = "lost";

const char SQUARE_EMPTY = 'E';
const char SQUARE_FILLED = 'F';
const char SQUARE_DESTROYED = 'D';
const char SQUARE_MISSED = 'M';
const char SQUARE_DEFAULT = 'R';

// Finished.
enum Position {
	EMPTY,
	FILLED,
	DESTROYED,
	MISSED,
};

// Finished.
enum Ship {
	BATTLESHIP = 4,
	CRUISER = 3,
	DESTROYER = 2,
	SUBMARINE = 1,
};

// Finished.
enum State {
	STATE_WAITING_PLAYERS,
	STATE_PLACING_SHIPS,
	STATE_FIGHTING,
	STATE_GAME_FINISHED,
};

// Was given shape clicked in these coords?
bool isClickedOnShape(sf::Vector2i hit_coords, sf::RectangleShape &shape) {
	int posX = shape.getPosition().x,
		posY = shape.getPosition().y;
	return
		hit_coords.x > posX &&
		hit_coords.x < posX + shape.getSize().x &&
		hit_coords.y > posY &&
		hit_coords.y < posY + shape.getSize().y;
}

// Return color by square state in char.
sf::Color getColorByChar(char chr) {
	switch (chr) {
	case SQUARE_DESTROYED:
		return sf::Color::Red;
	case SQUARE_EMPTY:
		return sf::Color::White;
	case SQUARE_FILLED:
		return sf::Color::Green;
	case SQUARE_MISSED:
		return sf::Color::Yellow;
	default:
		return sf::Color::Black;
	}
}

// Finished.
struct Player {
	std::vector<Ship> ships;
	Position Field[ROWS][COLS];
	std::string name;
	sf::TcpSocket *socket;

	Player() {
		name = "";
		for (int i = 0; i < COUNT_BATTLESHIPS; i++) {
			ships.push_back(Ship::BATTLESHIP);
		}
		for (int i = 0; i < COUNT_DESTROYERS; i++) {
			ships.push_back(Ship::DESTROYER);
		}
		for (int i = 0; i < COUNT_CRUISERS; i++) {
			ships.push_back(Ship::CRUISER);
		}
		for (int i = 0; i < COUNT_SUBMARINES; i++) {
			ships.push_back(Ship::SUBMARINE);
		}
	}
};

// Finished
struct Game {
	State gameState;
	std::vector<Player*> players;
	int playerTurn = 0;
};

// Finished.
Game game;

// Finished.
void resetField(Player *player) {
	for (int x = 0; x < COLS; x++) {
		for (int y = 0; y < ROWS; y++) {
			player->Field[y][x] = EMPTY;
		}
	}
}

// Finished
bool placeShip(Player *player, int x1, int x2, int y1, int y2) {
	int x_bigger = x1 > x2 ? x1 : x2;
	int y_bigger = y1 > y2 ? y1 : y2;

	int x_smaller = x_bigger == x1 ? x2 : x1;
	int y_smaller = y_bigger == y1 ? y2 : y1;

	if (x_bigger != x_smaller && y_bigger != y_smaller) {
		return false;
	}

	Ship shipPlaced = (Ship)(
		(x_bigger != x_smaller ?
			y_bigger - y_smaller : x_bigger - x_smaller)
		+ 1);

	bool shipWasntPlaced = false;

	for (auto it = player->ships.begin(); it != player->ships.end(); it++) {
		if (*it == shipPlaced) {
			shipWasntPlaced = true;
			player->ships.erase(it);
			break;
		}
	}

	if (!shipWasntPlaced) return false;

	for (int x = x_smaller; x <= x_bigger; x++) {
		for (int y = y_smaller; y <= y_bigger; y++) {
			if (x < 0 || x > COLS || y < 0 || y > ROWS) continue;
			//down
			if (player->Field[y + 1][x] == FILLED) {
				return false;
			}
			//downleft
			if (player->Field[y + 1][x - 1] == FILLED) {
				return false;
			}
			//left
			if (player->Field[y][x - 1] == FILLED) {
				return false;
			}
			//upleft
			if (player->Field[y - 1][x - 1] == FILLED) {
				return false;
			}
			//up
			if (player->Field[y - 1][x] == FILLED) {
				return false;
			}
			//upright
			if (player->Field[y - 1][x + 1] == FILLED) {
				return false;
			}
			//right
			if (player->Field[y][x + 1] == FILLED) {
				return false;
			}
			//downright
			if (player->Field[y + 1][x + 1] == FILLED) {
				return false;
			}
		}
	}

	for (int x = x_smaller; x <= x_bigger; x++) {
		for (int y = y_smaller; y <= y_bigger; y++) {
			player->Field[y][x] = FILLED;
		}
	}

	return true;
}

// Finished
char squareToChar(Position *position) {
	switch (*position) {
	case Position::EMPTY:
		return SQUARE_EMPTY;
	case Position::DESTROYED:
		return SQUARE_DESTROYED;
	case Position::FILLED:
		return SQUARE_FILLED;
	case Position::MISSED:
		return SQUARE_MISSED;
	default:
		return SQUARE_DEFAULT;
	}
}

// Finished.
void printSquare(Position *position) {
	std::cout << "[";
	std::cout << squareToChar(position);
	std::cout << "]";
}

// Finished.
void printFields() {
	std::cout << "\n\n           PLAYER 1                          PLAYER 2";
	for (int y = 0; y < ROWS; y++) {
		std::cout << "\n";
		for (int x = 0; x < COLS; x++) {
			printSquare(&(game.players[0]->Field[y][x]));
		}
		std::cout << "    ";
		for (int x = 0; x < COLS; x++) {
			printSquare(&(game.players[1]->Field[y][x]));
		}
	}
}

// Finished.
bool hitField(Position field[ROWS][COLS], int x, int y) {
	Position *p_pos = &field[y][x];
	if (*p_pos == EMPTY || *p_pos == FILLED) {
		field[y][x] = *p_pos == EMPTY ? MISSED : DESTROYED;
		return true;
	}
	return false;
}

// Finished.
bool noShipsLeft() {
	for (auto it = game.players.begin(); it != game.players.end(); it++) {
		if ((*it)->ships.size() != 0) return false;
	}
	return true;
}

// Finished.
bool fieldDestroyed(Position field[ROWS][COLS]) {
	for (int x = 0; x < COLS; x++) {
		for (int y = 0; y < ROWS; y++) {
			if (field[y][x] == FILLED) {
				return false;
			}
		}
	}
	return true;
}

// Finished.
int getNextPlayerIndex(int indexCurrent) {
	return (game.playerTurn + 1) % game.players.size();
}

// Finished.
int getPlayerIndex(Player *player) {
	for (int i = 0; i < game.players.size(); i++) {
		if (game.players[i] == player) {
			return i;
		}
	}
	return -1;
}

// Finished.
Player* getPlayerBySocket(sf::Socket *socket) {
	for (int i = 0; i < game.players.size(); i++) {
		if (game.players[i]->socket == socket) {
			return game.players[i];
		}
	}
	return nullptr;
}

// Finished.
std::string fieldsToStr(Player *player) {
	const int size = ROWS * COLS * 2;
	std::string returned;
	for (int x = 0; x < COLS; x++) {
		for (int y = 0; y < ROWS; y++) {
			returned.push_back(squareToChar( player->Field[y, x] ));
		}
	}
	Player *nextPlayer = game.players[0] == player ? game.players[1] : game.players[0];

	for (int x = 0; x < COLS; x++) {
		for (int y = 0; y < ROWS; y++) {
			returned.push_back(squareToChar( nextPlayer->Field[y, x] ));
		}
	}
	return returned;
}

// Finished.
std::string responceStr(Player *player, std::string receivedStr) {
	return fieldsToStr(player);
}

// Finished.
void sendMessage(sf::TcpSocket* socket, std::string msgStr) {
	char messageToSend[MAX_BUFF];

	for (int i = 0; i < MAX_BUFF; i++) {
		messageToSend[i] = '\0';
	}

	std::copy(msgStr.begin(), msgStr.end(), messageToSend);

	if (socket->send(messageToSend, MAX_BUFF) != sf::Socket::Done) {
		std::cout << "\nSend message failed.";
	}
	else {
		std::cout << "\nMessage sent: " + msgStr;
	}
}

// Finished.
void sendMessageClients(std::string msgStr) {
	for (int i = 0; i < game.players.size(); i++) {
		sendMessage(game.players[i]->socket, msgStr);
	}
}

// Finished.
void setFillColors(std::string fields) {
	int halfSize = 100;
	for (int i = 0; i < halfSize; i++) {
		_ownFieldSquaredRects[i].setFillColor(getColorByChar(fields[i]));
	}
	for (int i = halfSize; i < 200; i++) {
		_enemyFieldSquaredRects[i].setFillColor(getColorByChar(fields[i]));
	}
}

void receiveServerSocketHandler(std::reference_wrapper<sf::TcpSocket*> socket) {
	while (true) {
		char dataReceived[MAX_BUFF];
		size_t actualReceived = 0;

		for (int i = 0; i < MAX_BUFF; i++) {
			dataReceived[i] = '\0';
		}

		sf::TcpSocket* p_socket = socket.get();

		if (p_socket->receive(dataReceived, buffSize, actualReceived) != sf::Socket::Done)
		{
			std::cout << "\Server disconnected.";
			game.gameState = STATE_GAME_FINISHED;
			return;
		}
		else {
			std::cout << "\nText recieved: " << dataReceived;

			std::string receivedStr;
			receivedStr.append(dataReceived);

			if (receivedStr == CMD_PLACING_SHIPS) {
				std::cout << "\nPlacing ships.";
				game.gameState = STATE_PLACING_SHIPS;
			}
			else if (receivedStr == CMD_FIGHTING) {
				std::cout << "\nThe battle begins!";
				game.gameState = STATE_FIGHTING;
			}
			else if (receivedStr == CMD_GAME_FINISHED) {
				std::cout << "\nGame finished!";
				game.gameState = STATE_GAME_FINISHED;
			}
			else if (receivedStr == CMD_LOST) {
				std::cout << "\nYou lost :(";
			}
			else if (receivedStr == CMD_WON) {
				std::cout << "\nYou won :)";
			}
			else {
				//setFillColors(std::string(dataReceived));
				int halfSize = 100;
				for (int i = 0; i < halfSize; i++) {
					_ownFieldSquaredRects[i].setFillColor(getColorByChar(receivedStr[i]));
				}
				for (int i = halfSize; i < 200; i++) {
					_enemyFieldSquaredRects[i].setFillColor(getColorByChar(receivedStr[i]));
				}
			}
		}
	}
}

// Needs finish.
void receiveFromClientHandler(std::reference_wrapper<Player*> refPlayer) {

	Player* p_player = refPlayer.get();

	while (true) {
		char dataReceived[MAX_BUFF];
		size_t actualReceived = 0;

		for (int i = 0; i < MAX_BUFF; i++) {
			dataReceived[i] = '\0';
		}

		if (p_player->socket->receive(dataReceived, buffSize, actualReceived) != sf::Socket::Done)
		{
			std::cout << "\nClient disconnected.";
			game.gameState = STATE_GAME_FINISHED;
			return;
		}
		else {
			std::cout << "\nText recieved: " << dataReceived;

			std::string receivedStr;
			receivedStr.append(dataReceived);

			sendMessageClients(responceStr(p_player, receivedStr));
		}
	}
}

void startGameOnServer() {
	game.gameState = STATE_PLACING_SHIPS;
	game.playerTurn = 0;
	sendMessageClients(CMD_PLACING_SHIPS);

	//// VERY QUICK AND VERY DIRTY
	//while (game.gameState != STATE_GAME_FINISHED) {

	//}
}

// Finished.
void acceptClientHandler() {
	std::cout << "\nAccepting incoming connections.";
	sf::TcpSocket clientSocket;
	if (listener.accept(clientSocket) != sf::Socket::Done) {
		std::cout << "\nClient wasn't accepted.";
		acceptClientHandler();
	}
	else {
		std::cout << "\nClient accepted.";
		Player* player = &Player();
		player->socket = &clientSocket;
		resetField(player);
		game.players.push_back(player);
		
		std::thread receiveClientThread(receiveFromClientHandler, std::ref(player));
		receiveClientThread.detach();

		if (game.players.size() < 2) {
			std::cout << "\nFirst player connected, waiting for second to connect...";
			acceptClientHandler();
		}
		else {
			std::cout << "\nBoth players are connected, placing ships.";
			startGameOnServer();
		}
	}
}
// Finished.
void startServer() {
	listener.listen(53000, sf::IpAddress::getLocalAddress());

	acceptClientHandler();
}

// Update field visuals.
void redrawField(sf::RenderWindow &window) {
	window.clear();
	for (size_t y = 0; y < _squaresCount; y++) {
		for (size_t x = 0; x < _squaresCount; x++)
		{
			window.draw(_ownFieldSquaredRects[y * _squaresCount + x]);
			window.draw(_enemyFieldSquaredRects[y * _squaresCount + x]);
		}
	}
	window.display();
}

void createWindowGame() {
	// Renderable SFML window.
	sf::RenderWindow window(sf::VideoMode(_squaredField * 2 + _distanceBetweenFields + _pixelSizeSquaresOutline,
		_squaredField + _pixelSizeSquaresOutline
	), "Battleship Client");

	for (size_t y = 0; y < _squaresCount; y++) {
		for (size_t x = 0; x < _squaresCount; x++)
		{
			_ownFieldSquaredRects.push_back(sf::RectangleShape(sf::Vector2f(_pixelSizeSquare, _pixelSizeSquare)));
			_ownFieldSquaredRects[y * _squaresCount + x].setFillColor(sf::Color::White);
			_ownFieldSquaredRects[y * _squaresCount + x].setPosition(sf::Vector2f(_pixelSizeSquaresOutline + x * _pixelSizeBlockWithOutline, _pixelSizeSquaresOutline + y * _pixelSizeBlockWithOutline));

			_enemyFieldSquaredRects.push_back(sf::RectangleShape(sf::Vector2f(_pixelSizeSquare, _pixelSizeSquare)));
			_enemyFieldSquaredRects[y * _squaresCount + x].setFillColor(sf::Color::White);
			_enemyFieldSquaredRects[y * _squaresCount + x].setPosition(sf::Vector2f(_pixelSizeSquaresOutline + _offsetXPlayerfield + x * _pixelSizeBlockWithOutline, _pixelSizeSquaresOutline + y * _pixelSizeBlockWithOutline));
		}
	}
	redrawField(window);

	std::string lastPosPressed;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::MouseButtonPressed) {
				if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
					bool isShapeClicked = false;
					bool isEnemyClicked = false;
					sf::RectangleShape* clickedShape = nullptr;
					sf::Vector2i mousePos = sf::Mouse::getPosition(window);
					int x = 0;
					int y = 0;
					for (x = 0; x < _squaresCount; x++)
					{
						for (y = 0; y < _squaresCount; y++)
						{
							clickedShape = &_ownFieldSquaredRects[y * _squaresCount + x];
							if (isClickedOnShape(mousePos, *clickedShape)) {
								isShapeClicked = true;
								isEnemyClicked = false;
								break;
							}
							clickedShape = &_enemyFieldSquaredRects[y * _squaresCount + x];
							if (isClickedOnShape(mousePos, *clickedShape)) {
								isShapeClicked = true;
								isEnemyClicked = true;
								break;
							}
						}
						if (isShapeClicked) {
							break;
						}
					}

					if (isShapeClicked) {
						if (game.gameState == STATE_PLACING_SHIPS && !isEnemyClicked)
						{
							if (lastPosPressed.size() == 0) {
								lastPosPressed.append(std::to_string(x)).append(std::to_string(y));
								std::cout << lastPosPressed;
							}
							else {
								lastPosPressed.append(std::to_string(x)).append(std::to_string(y));
								sendMessage(&serverSocket, lastPosPressed);
								lastPosPressed.clear();
							}
						}
						else if (game.gameState == STATE_FIGHTING && isEnemyClicked) {
							sendMessage(&serverSocket, std::to_string(x).append(std::to_string(y)));
						}
					}
				}
			}
		}
		redrawField(window);
	}
}

// Needs finish.
void startClient() {
	std::cout << "\nInitializing client...";
	std::cout << "\nAttempting connect to server...";

	if (serverSocket.connect(sf::IpAddress::getLocalAddress(), 53000) != sf::Socket::Done)
	{
		std::cout << "\nCan't connect to server.";
		return;
	}
	else {
		std::cout << "\nConnected to server succesfully.";

		sf::TcpSocket* socketPointer = &serverSocket;

		std::thread receiveServerThread(receiveServerSocketHandler, std::ref(socketPointer));
		receiveServerThread.detach();

		createWindowGame();
	}
}

// Finished.
int main() {
	std::cout << "\nBattleship app starting.";
	std::cout << "\nChoose app mode [s] server [c] client: ";

	while (true) {
		std::string enteredStr;
		std::cin.clear();
		std::cin >> enteredStr;
		if (enteredStr == "s") {
			std::cout << "\nStarting a server...";
			startServer();
			break;
		}
		else if (enteredStr == "c") {
			std::cout << "\nStarting a client...";
			startClient();
			break;
		}
		else {
			std::cout << "\nUncorrect text entered. Write [s] for server or [c] for client: ";
		}
	}

	system("pause");
	return 0;
}