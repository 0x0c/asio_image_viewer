#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <cv.h>
#include <highgui.h>

namespace asio = boost::asio;
using asio::ip::tcp;

cv::Mat bytesToMat(unsigned char *bytes, uint32_t width, uint32_t height)
{
	cv::Mat image = cv::Mat(height, width, CV_8UC3);
	memcpy(image.data, bytes, width * height * 3);
    return image.clone();
}

int main(int argc, char **argv)
{
	for (int i = 0; i < argc; ++i) {
		printf("%s, ", argv[i]);
	}

	if (strcmp(argv[1], "-c") == 0 && argc == 4) {
		asio::io_service io_service;
		tcp::socket socket(io_service);
		boost::system::error_code error;

		int cnt = 0;
		while(1) {
			socket.connect(tcp::endpoint(asio::ip::address::from_string(std::string(argv[2])), atoi(argv[3])));
			io_service.run();

			std::string filename = "img/" + std::to_string(cnt % 31 + 1) + ".jpeg";
			// アルファチャネルを無視して読み込む
			cv::Mat image = cv::imread(filename, -1);

			// 画像のサイズ + x, y ,width, heightの4byte分だけ確保
			uint32_t size = image.total() * image.elemSize() + sizeof(uint32_t) * 4;
			unsigned char *bytes = new unsigned char[size];
			uint32_t x = 0;
			uint32_t y = 0;
			std::memcpy(bytes, &x, sizeof(uint32_t));
			std::memcpy(bytes + sizeof(uint32_t), &y, sizeof(uint32_t));
			std::memcpy(bytes + sizeof(uint32_t) * 2, &image.cols, sizeof(uint32_t));
			std::memcpy(bytes + sizeof(uint32_t) * 3, &image.rows, sizeof(uint32_t));
			std::memcpy(bytes + sizeof(uint32_t) * 4, image.data, image.total() * image.elemSize());

			std::cout << "send byte : " << size << std::endl;
			std::cout << "x : " << x << ", y : " << y << ", width : " << image.cols << ", height : " << image.rows << std::endl;
			asio::write(socket, asio::buffer(bytes, size), error);
			delete bytes;
			socket.close();
			usleep(100);
			cnt++;
		}
	}
	else if (strcmp(argv[1], "-s") == 0 && argc == 3){
		cv::namedWindow("received imate", CV_WINDOW_AUTOSIZE | CV_WINDOW_FREERATIO);
		asio::io_service io_service;
		tcp::acceptor acc(io_service, tcp::endpoint(tcp::v4(), atoi(argv[2])));
		tcp::socket socket(io_service);

		asio::streambuf receive_buffer;
		boost::system::error_code error;
		while(1) {
			// 接続待機
			acc.accept(socket);
			asio::socket_base::keep_alive option(true);
			socket.set_option(option);

			asio::read(socket, receive_buffer, asio::transfer_all(), error);
			if (error && error != asio::error::eof) {
				std::cout << "receive failed : " << error.message() << std::endl;
			}
			else {
				std::cout << "receive byte : " << receive_buffer.size() << std::endl;
				const unsigned char *data = asio::buffer_cast<const unsigned char *>(receive_buffer.data());
				uint32_t x, y;
				std::memcpy(&x, data, sizeof(uint32_t));
				std::memcpy(&y, data + sizeof(uint32_t), sizeof(uint32_t));

				uint32_t width, height;
				std::memcpy(&width, data + sizeof(uint32_t) * 2, sizeof(uint32_t));
				std::memcpy(&height, data + sizeof(uint32_t) * 3, sizeof(uint32_t));

				std::cout << "x : " << x << ", y : " << y << ", width : " << width << ", height : " << height << std::endl;
				if (height > 0 && width > 0) {
					cv::Mat image = bytesToMat((unsigned char *)data + sizeof(uint32_t) * 4, width, height);
					receive_buffer.consume(receive_buffer.size());
					cv::imshow("received image", image);
					cv::waitKey(60);
				}
			}
			socket.close();
		}
	}
}
