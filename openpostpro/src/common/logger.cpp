// OpenPostPro - www.rebisway.com - contact@rebisway.com
// Copyright (C) 2024-2026 Thomas Gourgnier
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "logger.h"
#include "date.h"
#include "environment.h"
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <chrono>

// _mutex protects the queue, _path, the thread, the running state and the flags; _file_mutex protects the file (written by the
// logging thread, and by any thread when always_flush is set). When both are needed, _mutex is taken first.
std::string Logger::_path;
static std::queue<std::string> _queue;
static std::mutex _mutex;
static std::mutex _file_mutex;
static std::condition_variable _wake;	// something to write, or a stop request
static std::condition_variable _idle;	// the queue is empty and nothing is being written
static std::filebuf _file;
static std::atomic<bool> _stop_requested{ false };
static std::thread* _thread = nullptr;
static bool _writing = false;			// the logging thread has taken lines it has not written yet
std::string Logger::_trace;
static std::mutex _trace_mutex;
static std::atomic<bool> _always_flush{ false };

static void write_line(std::ostream& os, const std::string& data)
{
	os << date::format("%F %T", std::chrono::system_clock::now()) << ": " << data << std::endl;
}

std::string Logger::path()
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _path;
}

std::string Logger::trace()
{
	std::lock_guard<std::mutex> lock(_trace_mutex);
	return _trace;
}

void Logger::trace(std::string data)
{
	std::lock_guard<std::mutex> lock(_trace_mutex);
	_trace = std::move(data);
}

void Logger::log(std::string data)
{
	if (_always_flush) // debug purpose
	{
		std::lock_guard<std::mutex> lock(_file_mutex);
		if (_file.is_open())
		{
			std::ostream os(&_file);
			write_line(os, data);
			os.flush();
		}
	}
	else
	{
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_queue.push(std::move(data));
		}
		_wake.notify_one();
	}
}

void Logger::error(std::string data)
{
	log("[ERROR]: " + data);
}

void Logger::success(std::string data)
{
	log("[SUCCESS]: " + data);
}

void Logger::warning(std::string data)
{
	log("[WARNING]: " + data);
}

void Logger::flush()
{
	std::unique_lock<std::mutex> lock(_mutex);
	_wake.notify_one();
	// without the thread nothing would ever empty the queue: do not wait for it
	if (_thread == nullptr)
		return;
	_idle.wait_for(lock, std::chrono::seconds(5), [] { return _queue.empty() && !_writing; });
}

void Logger::clear()
{
	std::string path = Logger::path();
	bool flush_mode = _always_flush;
	stop();
	std::error_code ec;
	std::filesystem::remove(path, ec);
	start(path, flush_mode);
}

void Logger::loop()
{
	std::ostream os(&_file);
	{
		std::lock_guard<std::mutex> lock(_file_mutex);
		os << date::format("%F %T", std::chrono::system_clock::now()) << ": " << "--------------- START ----------------\n";
		os.flush();
	}

	bool stopping = false;
	while (!stopping)
	{
		std::queue<std::string> batch;
		{
			std::unique_lock<std::mutex> lock(_mutex);
			_wake.wait_for(lock, std::chrono::milliseconds(250), [] { return _stop_requested.load() || !_queue.empty(); });
			std::swap(batch, _queue);
			// the last round (nothing left after the stop request) catches the lines logged during the previous write
			stopping = _stop_requested && batch.empty();
			_writing = !batch.empty();
		}

		// write to file, without holding the queue mutex: the loggers are not blocked by the disk
		try {
			std::lock_guard<std::mutex> lock(_file_mutex);
			while (!batch.empty())
			{
				write_line(os, batch.front());
				batch.pop();
			}
			os.flush();
		}
		catch (...) {
		}

		{
			std::lock_guard<std::mutex> lock(_mutex);
			_writing = false;
			// lines logged while writing are for the next round, flush() waits for them
			if (_queue.empty())
				_idle.notify_all();
		}
	}

	{
		std::lock_guard<std::mutex> lock(_file_mutex);
		os << date::format("%F %T", std::chrono::system_clock::now()) << ": " << "---------------- END -----------------\n";
		os.flush();
		_file.close();
	}
}

void Logger::start(std::string path, bool always_flush)
{
	// already running: nothing to start
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_thread != nullptr)
			return;
	}

	if (path == "")
	{
		path = "log.txt";
	}
	else
	{
		// check if folder exists
		std::error_code ec;
		std::filesystem::path folder = std::filesystem::path(path).parent_path();
		if (!folder.empty())
			std::filesystem::create_directories(folder, ec);
	}

	{
		std::lock_guard<std::mutex> lock(_file_mutex);
		_file.open(path, std::ios::out | std::ios::app);
	}

	std::lock_guard<std::mutex> lock(_mutex);
	_always_flush = always_flush;
	_stop_requested = false;
	_path = path;
	_thread = new std::thread(&loop);
}


void Logger::stop()
{
	std::thread* thread;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		thread = _thread;
		_thread = nullptr;
		_stop_requested = true;
	}
	_wake.notify_all();
	_idle.notify_all();
	if (thread == nullptr)
		return;
	thread->join();
	delete thread;
}
