#pragma once

#include <concepts>
#include <pthread.h>
#include <thread>
#include <list>
#include <vector>
#include <csignal>

#include <pthread.h>

#include <fmt/printf.h>

#include "../../wrapped_types.hpp"

namespace jvl::gfx::cpu {

struct worker_status {
	bool begin = false;
	bool done = false;
	bool kill = false;
};

template <typename T, typename F>
requires std::same_as <std::invoke_result_t <F, T>, void>
void worker_loop(const F &kernel, wrapped::thread_safe_queue <T> &queue, worker_status &status)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);

	// TODO: sleep
	while (!status.begin) {
		if (status.kill)
			return;
	}

	while (!status.kill) {
		T item;

		{
			std::lock_guard guard(queue.lock);
			if (queue.size_locked())
				item = queue.pop_locked();
			else
				break;
		}

		// TODO: message method in sync
		kernel(item);
	}

	fmt::println("kill status: {}", status.kill);

	status.done = true;
}

template <typename T, typename F>
class fixed_function_thread_pool {
	F kernel;
	size_t count;

	wrapped::thread_safe_queue <T> queue;

	std::list <worker_status> status;
	std::vector <std::thread> threads;

	void __init() {
		for (size_t i = 0; i < count; i++) {
			worker_status s;
			status.push_back(s);
			threads.emplace_back(worker_loop <T, F>, kernel, std::ref(queue), std::ref(status.back()));
		}
	}
public:
	fixed_function_thread_pool(const fixed_function_thread_pool &) = delete;
	fixed_function_thread_pool &operator=(const fixed_function_thread_pool &) = delete;

	fixed_function_thread_pool(size_t count_, const F &kernel_)
			: kernel(kernel_), count(count_) {
		__init();
	}

	void enque(const wrapped::thread_safe_queue <T> &items) {
		queue.push(items);
	}

	void begin() {
		for (auto &s : status)
			s.begin = true;
	}

	bool done() const {
		for (auto &s : status) {
			if (!s.done)
				return false;
		}

		return true;
	}

	void reset() {
		auto it = status.begin();
		for (size_t i = 0; i < count; i++) {
			auto &t = threads[i];
			auto &s = *it++;

			if (s.done) {
				t.join();
				continue;
			}

			pthread_t tid = t.native_handle();
			t.detach();

			pthread_cancel(tid);
		}

		status.clear();
		threads.clear();
		queue.clear();

		__init();
	}
};

} // namespace jvl::gfx::cpu
