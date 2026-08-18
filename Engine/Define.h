#pragma once

#define DECLARE_SINGLE(classname)			\
private:									\
	classname() { }							\
public:										\
	static classname* GetInstance()			\
	{										\
		static classname s_instance;		\
		return &s_instance;					\
	}

#define DECLARE_SINGLE_WITH_CONSTRUCTOR(classname)\
private:									\
	classname();							\
public:										\
	static classname* GetInstance()			\
	{										\
		static classname s_instance;		\
		return &s_instance;					\
	}

#define GET_SINGLE(classname)	classname::GetInstance()

#define CHECK(p)	assert(SUCCEEDED(p))
#define GAME		GET_SINGLE(Game)		
#define GRAPHICS	GET_SINGLE(Graphics)
#define DEVICE		GRAPHICS->GetDevice()
#define DC			GRAPHICS->GetDeviceContext()
#define INPUT		GET_SINGLE(InputManager)
#define TIME		GET_SINGLE(TimeManager)
#define DT			TIME->GetDeltaTime()
#define TWEEN		GET_SINGLE(TweenManager)
#define RESOURCES	GET_SINGLE(ResourceManager)
#define EDITOR		GET_SINGLE(EditorManager)
#define SCENE		GET_SINGLE(SceneManager)
#define CUR_SCENE	SCENE->GetCurrentScene()
#define DBG			GET_SINGLE(DebugManager) // DEBUG라는 디파인은 있는 경우가 많으므로 DBG로 줄임

#define U8(str) reinterpret_cast<const char*>(u8##str)

enum Layer_Mask
{
	Layer_Default = 0,
	Layer_UI = 1
};

#ifdef _DEBUG
#define ASSERT(cond, msg) \
    Assert((cond), #cond, (msg), __FILE__, __LINE__)

#define ASSERT(cond) \
    Assert((cond), #cond, __FILE__, __LINE__)

#else
#define ASSERT(cond, msg) ((void)0)
#define ASSERT(cond) ((void)0)
#endif