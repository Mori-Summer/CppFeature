#include <iostream>
#include <memory>


/// 最核心的就是抽象与实现分离，这样抽象和实现修改起来就都很方便
/// 没有继承关系的限制


// 具体实现类抽象接口
class Implementor
{
public:
	virtual void OperationImpl() = 0;
};

// 具体实现
class ConcreteImpementor : public Implementor
{
public:
	void OperationImpl() override
	{
		std::cout << "OperationImpl" << std::endl;
	}
};

// 抽象类接口 -- 桥接器
class Abstraction
{
public:
	Abstraction(std::shared_ptr<Implementor>& p) : m_pImpl(p) {};
	virtual void Operation() = 0;


protected:
	// 具体功能类指针，要桥接多个就添加多个
	std::shared_ptr<Implementor> m_pImpl;
};

// 实际桥接器
class RedfinedAbstrction : public Abstraction
{
public:
	RedfinedAbstrction(std::shared_ptr<Implementor>& p) : Abstraction(p) {};

	// 桥接功能
	void Operation() override
	{
		m_pImpl->OperationImpl();
	}
};

// 实际例子：

//  一个可以画各种颜色的画笔
class AbsColorPainter
{
public:
	virtual void PaintColor() = 0;
	virtual ~AbsColorPainter() {};
};

class BlueColorPainter : public AbsColorPainter
{
public:
	void PaintColor() override
	{
		std::cout << "paintColor: Blue" << std::endl;
	}
};

class RedColorPainter : public AbsColorPainter
{
public:
	void PaintColor() override
	{
		std::cout << "paintColor: Red" << std::endl;
	}
};

// 一个可以画各种形状的画笔
class AbsShapePainter
{
public:
	virtual void PaintShape() = 0;
	virtual ~AbsShapePainter() {};

	AbsShapePainter(std::shared_ptr<AbsColorPainter>& p) : m_pColorPainter(p) {};

protected:
	// 这里我们把颜色画笔桥接进形状画笔中，这样以后不管是新增颜色还是新增形状
	// 就都不需要新增额外的操作，只需要分别新增实现的类即可
	std::shared_ptr<AbsColorPainter> m_pColorPainter;
};

class CubeShapePainter : public AbsShapePainter
{
public:
	using AbsShapePainter::AbsShapePainter;

	void PaintShape() override
	{
		m_pColorPainter->PaintColor();
		std::cout << "paintShape: Cube" << std::endl;
	}
};
