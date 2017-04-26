#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <thread>
#include <vector>

enum State {
	STATE_WAITING_FOR_GAME,
	STATE_GAME_PLACING_SHIPS,
	STATE_GAME_FIGHTING,
	STATE_GAME_FINISHED,
};

State _state = State::STATE_WAITING_FOR_GAME;

sf::TcpSocket _socket;
std::vector<sf::RectangleShape> _own_squares,			// Own client's field squares
_enemy_squares;										// Enemy field squares.

int _squares_count = 10,
_pixel_size_square = 50,
_pixel_size_squares_outline = 1,
_pixel_size_block_with_outline = _pixel_size_square + _pixel_size_squares_outline,
_squared_field = _pixel_size_block_with_outline * _squares_count,
_distance_between_fields = 10,
_offset_x_playerfield = _squared_field + _distance_between_fields;

bool clicked_on_shape(sf::Vector2i hit_coords, sf::RectangleShape &shape) {
	int posX = shape.getPosition().x,
		posY = shape.getPosition().y;
	return
		hit_coords.x > posX &&
		hit_coords.x < posX + shape.getSize().x &&
		hit_coords.y > posY &&
		hit_coords.y < posY + shape.getSize().y;
}

sf::Color getColorByChar(char *chr) {
	switch (*chr) {
	case 'D':
		return sf::Color::Red;
	case 'E':
		return sf::Color::White;
	case 'F':
		return sf::Color::Green;
	case 'M':
		return sf::Color::Yellow;
	default:
		return sf::Color::Black;
	}
}

void receiveHandler() {
	while (true) {
		char dataReceived[400];
		std::size_t received = 400;

		for (int i = 0; i < 400; i++) {
			dataReceived[i] = '\0';
		}

		if (_socket.receive(dataReceived, 400, received) != sf::Socket::Done)
		{
			std::cout << "\nRecieve from server failed.";
		}
		else {
			std::cout << '\n' << "Text received from server: " << dataReceived;
		}
	}
}

void sendMessageServer(char *message) {
	char dataSent[400];
	std::size_t sent = 400;

	for (int i = 0; i < 400; i++) {
		dataSent[i] = '\0';
	}

	strcpy(dataSent, message);

	std::cout << "\nAttempting send data to server: " << dataSent;

	if (_socket.send(dataSent, 400, sent) != sf::Socket::Done)
	{
		std::cout << "\nSend text to server failed.";
		return;
	}
	else {
		std::cout << "\nText sent to server successfully.";
	}
}

void redrawField(sf::RenderWindow &window) {
	window.clear();
	for (size_t y = 0; y < _squares_count; y++) {
		for (size_t x = 0; x < _squares_count; x++)
		{
			window.draw(_own_squares[y * _squares_count + x]);
			window.draw(_enemy_squares[y * _squares_count + x]);
		}
	}
	window.display();
}

int main()
{
	std::cout << "\nInitializing client...";
	std::cout << "\nAttempting connect to server...";
	if (_socket.connect(sf::IpAddress::getLocalAddress(), 53000) != sf::Socket::Done)
	{
		std::cout << "\nCan't connect to server.";
		return 0;
	}
	else {
		std::cout << "\nConnected to server succesfully.";
	}

	sf::RenderWindow window(sf::VideoMode(_squared_field * 2 + _distance_between_fields + _pixel_size_squares_outline,
		_squared_field + _pixel_size_squares_outline // up and down outline
	), "Battleship");
	for (size_t y = 0; y < _squares_count; y++) {
		for (size_t x = 0; x < _squares_count; x++)
		{
			_own_squares.push_back(sf::RectangleShape(sf::Vector2f(_pixel_size_square, _pixel_size_square)));
			_own_squares[y * _squares_count + x].setFillColor(sf::Color::White);
			_own_squares[y * _squares_count + x].setPosition(sf::Vector2f(_pixel_size_squares_outline + x * _pixel_size_block_with_outline, _pixel_size_squares_outline + y * _pixel_size_block_with_outline));

			_enemy_squares.push_back(sf::RectangleShape(sf::Vector2f(_pixel_size_square, _pixel_size_square)));
			_enemy_squares[y * _squares_count + x].setFillColor(sf::Color::White);
			_enemy_squares[y * _squares_count + x].setPosition(sf::Vector2f(_pixel_size_squares_outline + _offset_x_playerfield + x * _pixel_size_block_with_outline, _pixel_size_squares_outline + y * _pixel_size_block_with_outline));
		}
	}
	redrawField(window);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::MouseButtonPressed) {
				//std::cout << "mouse clicked";
				bool clicked = false;
				for (int y = 0; y < _squares_count; y++) {
					for (int x = 0; x < _squares_count; x++) {
						sf::RectangleShape * enemy = &_enemy_squares[y * _squares_count + x];

						if (clicked_on_shape(sf::Mouse::getPosition(window), *enemy)) {
							//std::cout << x << y;
							////enemy->setFillColor(sf::Color::Red);
							//switch (_state) {
							//case State::STATE_GAME_FIGHTING:

							//	char hit[2];
							//	hit[0] = x - '0';
							//	hit[1] = y - '0';

							//	sendMessageServer(hit);
							//	break;
							//}

							//break;
							//clicked = true;

							//sf::RectangleShape * own = &_own_squares[y * _squares_count + x];

							//if (clicked_on_shape(sf::Mouse::getPosition(window), *own)) {
							//	char hit[2];
							//	hit[0] = x - '0';
							//	hit[1] = y - '0';
							//	clicked = true;
							//	break;
							//}
						}
						if (clicked) break;
					}
				}
			}
			redrawField(window);
		}

		/*while (true) {
			char message[400];
			std::cout << "\nEnter text to send on server: ";
			std::cin >> message;
			sendMessageServer(message);
		}*/

		std::cout << '\n';
		system("pause");
		return 0;
	}
}