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

#pragma once
#include <algorithm>
#include <atomic>
#include <exception>
#include <thread>
#include <vector>

// Runs f(0) ... f(count - 1) on several threads (the calling thread works too), and returns when all are done. The calls must not share
// data which is written (each one writes its own result, in a slot given by its index). The first exception of a call is thrown again here.
// parallel_limit: number of threads at most, 0 is the number of cores, 1 is a plain loop (tests, comparison with the parallel result).
inline std::atomic<unsigned>& parallel_limit()
{
	static std::atomic<unsigned> limit{ 0 };
	return limit;
}

template <class F>
void parallel_for(size_t count, F&& f)
{
	unsigned threads = parallel_limit().load();
	if (threads == 0)
		threads = (std::max)(1u, std::thread::hardware_concurrency());
	threads = (unsigned)(std::min)((size_t)threads, count);
	if (threads <= 1)
	{
		for (size_t i = 0; i < count; i++)
			f(i);
		return;
	}

	std::atomic<size_t> next{ 0 };
	std::exception_ptr error;
	std::atomic<bool> failed{ false };
	auto work = [&]()
	{
		for (size_t i = next++; i < count && !failed; i = next++)
		{
			try
			{
				f(i);
			}
			catch (...)
			{
				if (!failed.exchange(true))
					error = std::current_exception();
			}
		}
	};
	std::vector<std::thread> pool;
	for (unsigned t = 1; t < threads; t++)
		pool.emplace_back(work);
	work();
	for (std::thread& t : pool)
		t.join();
	if (error)
		std::rethrow_exception(error);
}
