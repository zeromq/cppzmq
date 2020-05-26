#include <future>
#include <string>

#include "zmq.hpp"

#ifdef _WIN32
int main();
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	main();
}
#endif // _WIN32

#ifndef OutputDebugString
#include <iostream>
void OutputDebugString(char* s)
{
	std::cout << s;
}
#endif // OutputDebugString

void PublisherThread(zmq::context_t* ctx)
{
	//  Prepare publisher
	zmq::socket_t publisher(*ctx, zmq::socket_type::pub);
	publisher.bind("inproc://#1");

	while (true) {
		//  Write three messages, each with an envelope and content
		publisher.send(zmq::str_buffer("A"), zmq::send_flags::sndmore);
		publisher.send(zmq::str_buffer("Message in A envelope"));
		publisher.send(zmq::str_buffer("B"), zmq::send_flags::sndmore);
		publisher.send(zmq::str_buffer("Message in B envelope"));
		publisher.send(zmq::str_buffer("C"), zmq::send_flags::sndmore);
		publisher.send(zmq::str_buffer("Message in C envelope"));
		Sleep(100);
	}
}

void SubscriberThread1(zmq::context_t* ctx)
{
	//  Prepare subscriber
	zmq::socket_t subscriber(*ctx, zmq::socket_type::sub);
	subscriber.connect("inproc://#1");

	//  Thread2 opens "A" and "B" envelopes
	subscriber.setsockopt(ZMQ_SUBSCRIBE, "A", 1);
	subscriber.setsockopt(ZMQ_SUBSCRIBE, "B", 1);

	while (1) {
		//  Read envelope address
		zmq::message_t address;
		zmq::recv_result_t result = subscriber.recv(address);

		//  Read message contents
		zmq::message_t contents;
		result = subscriber.recv(contents);

		std::stringstream out;
		out << "Thread2: [" << address.to_string_view() << "] " << contents.to_string_view() << std::endl;
		OutputDebugString(out.str().c_str());
	}
}

void SubscriberThread2(zmq::context_t* ctx)
{
	//  Prepare our context and subscriber
	zmq::socket_t subscriber(*ctx, zmq::socket_type::sub);
	subscriber.connect("inproc://#1");

	//  Thread3 opens ALL envelopes
	subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);

	while (1) {
		//  Read envelope address
		zmq::message_t address;
		zmq::recv_result_t result = subscriber.recv(address);

		//  Read message contents
		zmq::message_t contents;
		result = subscriber.recv(contents);

		std::stringstream out;
		out << "Thread3: [" << address.to_string_view() << "] " << contents.to_string_view() << std::endl;
		OutputDebugString(out.str().c_str());
	}
}

int main()
{
	/*
	 * No I/O threads are involved in passing messages using the inproc transport.
	 * Therefore, if you are using a ØMQ context for in-process messaging only you
	 * can initialise the context with zero I/O threads.
	 *
	 * Source: http://api.zeromq.org/2-1:zmq-inproc
	 */
	zmq::context_t ctx;

	auto thread1 = std::async(std::launch::async, PublisherThread, &ctx);
	auto thread2 = std::async(std::launch::async, SubscriberThread1, &ctx);
	auto thread3 = std::async(std::launch::async, SubscriberThread2, &ctx);
	thread1.wait();
	thread2.wait();
	thread3.wait();

	/*
	 * Output:
	 *   An infinite loop of a mix of:
	 *     Thread2: [A] Message in A envelope
	 *     Thread2: [B] Message in B envelope
	 *     Thread3: [A] Message in A envelope
	 *     Thread3: [B] Message in B envelope
	 *     Thread3: [C] Message in C envelope
	 */
}
