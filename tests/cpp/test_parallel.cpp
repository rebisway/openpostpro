// PARALLEL TEST - the parts of a pocket computed on several threads give the same toolpath as a plain loop.
//
// What for:
//   A pocket made of several independent parts (a square with letters inside, each letter and its inside) is computed one part per
//   thread. The result must be the same curves, in the same order, as the sequential loop: the post-processor machines them in this
//   order (the finishing pass at the end). A shared variable which is written by two threads would give a different path or a crash.
//
// What is checked: for each pocket of data/pocket_text.opp (or the file given as argument), computed with one thread and with all the
//   cores, the same number of curves, the same points, the same count of finishing curves; and that parallel_for runs every index once
//   and gives back the exception of a call.
//
// Build: tests/cpp/build_parallel.bat   Run: tests/cpp/test_parallel.exe   (exit code 0 = all checks pass)
#include <pocket.h>
#include <parallel.h>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

struct TestRenderer : Renderer
{
	Buffer* create_buffer(std::vector<glm::vec3> vertices, std::vector<glm::vec3> normales = std::vector<glm::vec3>(), int usage = 0) override { return new Buffer(); }
};

static double now_ms()
{
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

int main(int argc, char** argv)
{
	int failures = 0;

	// parallel_for
	{
		std::vector<int> hits(1000, 0);
		parallel_for(hits.size(), [&](size_t i) { hits[i]++; });
		bool once = true;
		for (int h : hits) once &= h == 1;
		if (!once) { std::printf("FAIL: an index is not run exactly once\n"); failures++; }
		bool thrown = false;
		try { parallel_for(50, [&](size_t i) { if (i == 7) throw std::runtime_error("x"); }); }
		catch (const std::runtime_error&) { thrown = true; }
		if (!thrown) { std::printf("FAIL: the exception of a call is lost\n"); failures++; }
		parallel_for(0, [&](size_t) { failures += 100; });
	}

	std::string path = std::string(__FILE__).substr(0, std::string(__FILE__).find_last_of("/\\") + 1) + "data/pocket_text.opp";
	if (argc > 1)
		path = argv[1];
	std::ifstream in(path);
	if (!in)
	{
		std::printf("file not found: %s\n", path.c_str());
		return 2;
	}
	static TestRenderer renderer;
	static Camera camera;
	renderer.camera(&camera);

	std::string line;
	bool toolpath = false;
	int pockets = 0;
	while (std::getline(in, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line == "{TOOLPATH}") { toolpath = true; continue; }
		if (!toolpath || line.rfind("205;", 0) != 0)
			continue;

		Pocket sequential(&renderer), parallel(&renderer);
		sequential.read(line);
		parallel.read(line);
		parallel_limit() = 1;
		double t0 = now_ms();
		sequential.update();
		double t1 = now_ms();
		parallel_limit() = 0;
		parallel.update();
		double t2 = now_ms();

		auto a = sequential.coordinates(), b = parallel.coordinates();
		bool same = a.size() == b.size() && sequential.final_count() == parallel.final_count();
		for (size_t i = 0; same && i < a.size(); i++)
		{
			auto pa = a[i].coordinates(), pb = b[i].coordinates();
			same = pa.size() == pb.size();
			for (size_t k = 0; same && k < pa.size(); k++)
				same = pa[k] == pb[k];
		}
		std::printf("pocket %d: %zu curves, 1 thread %.0f ms, %u cores %.0f ms: %s\n", pockets, a.size(), t1 - t0, std::thread::hardware_concurrency(), t2 - t1, same ? "same" : "DIFFERENT");
		if (!same)
			failures++;
		pockets++;
	}
	std::printf("%d pockets, %d failures\n", pockets, failures);
	return failures == 0 && pockets > 0 ? 0 : 1;
}
