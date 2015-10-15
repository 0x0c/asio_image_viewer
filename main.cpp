#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <cv.h>
#include <highgui.h>

namespace asio = boost::asio;
using asio::ip::tcp;

cv::Mat bytesToMat(unsigned char *bytes, int width, int height)
{
	cv::Mat image = cv::Mat(height, width, CV_8UC3);
	memcpy(image.data, bytes, width * height * 3);
    return image.clone();
}

int main(int argc, char **argv)
{
	if (strcmp(argv[1], "c") == 0) {
		asio::io_service io_service;
		tcp::socket socket(io_service);
		boost::system::error_code error;

		int cnt = 0;
		while(1) {
			socket.connect(tcp::endpoint(asio::ip::address::from_string("172.27.254.26"), 31400));
			io_service.run();

			std::string filename = "img/" + std::to_string(cnt % 30 + 1) + ".jpeg";
			cv::Mat image = cv::imread(filename, -1);
			int size = image.total() * image.elemSize();
			std::cout << "send byte : " << size << std::endl;
			unsigned char *bytes = new unsigned char[size];
			std::memcpy(bytes, image.data, size);
			asio::write(socket, asio::buffer(bytes, size), error);
			delete bytes;
			socket.close();
			usleep(100);
			cnt++;
		}
	}
	else {
		cv::namedWindow("received imate", CV_WINDOW_AUTOSIZE | CV_WINDOW_FREERATIO);
		asio::io_service io_service;
		tcp::acceptor acc(io_service, tcp::endpoint(tcp::v4(), 31400));
		tcp::socket socket(io_service);

		asio::streambuf receive_buffer;
		boost::system::error_code error;
		while(1) {
			// 接続待機
			acc.accept(socket);
			asio::socket_base::keep_alive option(true);
			socket.set_option(option);
			std::cout << "accept" << std::endl;

			asio::read(socket, receive_buffer, asio::transfer_all(), error);
			if (error && error != asio::error::eof) {
				std::cout << "receive failed : " << error.message() << std::endl;
			}
			else {
				std::cout << "receive byte : " << receive_buffer.size() << std::endl;
				const unsigned char *data = asio::buffer_cast<const unsigned char *>(receive_buffer.data());
				cv::Mat image = bytesToMat((unsigned char *)data, 485, 314);
				receive_buffer.consume(receive_buffer.size());
				cv::imshow("received image", image);
				cv::waitKey(60);
			}
			socket.close();
		}
	}
}
