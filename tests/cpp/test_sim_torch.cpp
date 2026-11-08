// SIM TORCH TEST - a torch cuts through the block even when it stays above the top (cutting height of 0.5 mm, no contact start).
//
// What for:
//   A plasma torch is kept some distance over the sheet: the program never goes below the top of the block. The simulator must still
//   open the material from the top to the bottom under the torch, so that the cut can be seen through.
//
// What is checked: for every model, a block with `through`, a plasma tool going along a line at 0.5 mm over the top: the columns under
//   the line are at the bottom, the ones far from it are untouched. A flat tool at the same height does not cut anything.
//
// Build: tests/cpp/build_sim_torch.bat   Run: tests/cpp/test_sim_torch.exe   (exit code 0 = all checks pass)
#include "sim_model.h"
#include <cstdio>
#include <cmath>
#include <memory>

static int checks = 0, failed = 0;
#define CHECK(condition, text) do { checks++; if (!(condition)) { failed++; std::printf("  FAILED: %s\n", text); } } while (0)

int main()
{
	for (int m = 0; m < SimModel::method_count(); m++)
	{
		std::printf("method %s\n", SimModel::method_name(m));
		for (int pass = 0; pass < 2; pass++)
		{
			const bool plasma = pass == 0;
			SimTool tool;
			tool.shape = plasma ? SimToolShape::Plasma : SimToolShape::Flat;
			tool.radius = 0.75;
			tool.length = 20;
			SimBlock block;
			block.low = glm::dvec2(0, 0);
			block.high = glm::dvec2(100, 100);
			block.top = 0;
			block.bottom = -6;
			block.tool_radius = tool.radius;
			block.through = plasma;
			block.plateaus = true;
			std::unique_ptr<SimModel> model(SimModel::create(m));
			model->setup(block);
			model->cut(glm::dvec3(10, 50, 0.5), glm::dvec3(90, 50, 0.5), tool);
			model->cut(glm::dvec3(50, 10, 5), glm::dvec3(50, 90, 0.5), tool);	// along a ramp of the torch above the top
			if (plasma)
			{
				CHECK(model->height_at(30, 50) < -5.9, "the torch opens the material under its path (at 0.5 over the top)");
				CHECK(model->height_at(50, 30) < -5.9, "the torch opens the material under a move which goes down to 0.5 over the top");
				CHECK(model->height_at(30, 60) > -0.01, "the material far from the path is untouched");
			}
			else
				CHECK(model->height_at(30, 50) > -0.01, "a flat tool above the top does not cut");
		}
	}
	std::printf("%d checks, %d failed\n", checks, failed);
	return failed == 0 ? 0 : 1;
}
