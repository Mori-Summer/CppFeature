#include "DesignPattern/Bridge.hpp"

int main()
{
	std::shared_ptr<AbsColorPainter> baseTest = std::make_shared<RedColorPainter>();
	std::shared_ptr<AbsShapePainter> test = std::make_shared<CubeShapePainter>(baseTest);

	test->PaintShape();




	return 0;
}