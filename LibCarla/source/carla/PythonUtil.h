// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once  // 防止头文件被重复包含

#include "carla/NonCopyable.h"  // 包含Carla的非可复制类定义，可能用于防止类的实例被复制 

#ifdef LIBCARLA_WITH_PYTHON_SUPPORT  // 检查是否定义了LIBCARLA_WITH_PYTHON_SUPPORT宏，该宏通常用于控制是否包含Python支持  
#  if defined(__clang__)   // 如果使用Clang编译器，则保存当前的编译诊断设置
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wdeprecated-register"  // 忽略Clang编译器关于"-Wdeprecated-register"的警告，这通常是因为使用了已弃用的寄存器关键字 
#  endif
#    include <boost/python.hpp> // 包含Boost.Python的头文件，用于在C++代码中提供Python绑定 
#  if defined(__clang__)  // 如果之前是为了Clang编译器保存了编译诊断设置，现在恢复它们
#    pragma clang diagnostic pop
#  endif  // 结束LIBCARLA_WITH_PYTHON_SUPPORT宏的检查
#endif //用于在编译时启动LibCarla中与Python绑定的功能
namespace carla {

  class PythonUtil {
  public:

    // 检查当前线程是否持有Python全局解释器锁（GIL）
    static bool ThisThreadHasTheGIL() {
#ifdef LIBCARLA_WITH_PYTHON_SUPPORT  // 如果项目配置了Python支持
#  if PY_MAJOR_VERSION >= 3 // 如果是Python 3及以上版本
      return PyGILState_Check();  // 使用Python 3的API检查当前线程是否持有GIL
  #  else// 对于Python 2通过检查当前线程的状态是否等于GIL状态来检查
      PyThreadState *tstate = _PyThreadState_Current;
      return (tstate != nullptr) && (tstate == PyGILState_GetThisThreadState());
#  endif // PYTHON3
#else
      return false;  // 如果没有配置Python支持，总是返回false 
#endif // LIBCARLA_WITH_PYTHON_SUPPORT
    }

#ifdef LIBCARLA_WITH_PYTHON_SUPPORT

    /// 获取Python全局解释器锁（GIL），这是从其他线程调用Python代码所必需的。
    class AcquireGIL : private NonCopyable {
    public:

      // 构造函数确保当前线程获得GIL
      AcquireGIL() : _state(PyGILState_Ensure()) {}

      // 析构函数释放GIL
      ~AcquireGIL() {
        PyGILState_Release(_state);
      }

    private:

      PyGILState_STATE _state;  // 保存GIL的状态
    };

    /// 释放Python全局解释器锁（GIL），在执行阻塞I/O操作时使用它。
    class ReleaseGIL : private NonCopyable {
    public:

      // 构造函数保存当前线程的状态并释放GIL
      ReleaseGIL() : _state(PyEval_SaveThread()) {}

      // 析构函数恢复线程的状态
      ~ReleaseGIL() {
        PyEval_RestoreThread(_state);
      }

    private:

      PyThreadState *_state;  // 保存线程状态
    };

#else // LIBCARLA_WITH_PYTHON_SUPPORT

    // 如果没有Python支持，则定义空的AcquireGIL和ReleaseGIL类
    class AcquireGIL : private NonCopyable {};
    class ReleaseGIL : private NonCopyable {};

#endif // LIBCARLA_WITH_PYTHON_SUPPORT

    /// 可以传递给智能指针的删除器，在销毁对象之前确保获取GIL。
    class AcquireGILDeleter {
    public:
    // 调用delete删除ptr指向的对象，如果支持Python则确保在有GIL的情况下进行
      template <typename T>
      void operator()(T *ptr) const {
#ifdef LIBCARLA_WITH_PYTHON_SUPPORT
        if (ptr != nullptr && !PythonUtil::ThisThreadHasTheGIL()) {
          AcquireGIL lock;  // 获取GIL
          delete ptr;       // 删除对象
        } else
#endif // LIBCARLA_WITH_PYTHON_SUPPORT
        delete ptr;  // 如果没有Python支持，直接删除对象
      }
    };

    /// 可以传递给智能指针的删除器，在销毁对象之前释放GIL。
    class ReleaseGILDeleter {
    public:

      template <typename T>
      void operator()(T *ptr) const {
#ifdef LIBCARLA_WITH_PYTHON_SUPPORT
        if (ptr != nullptr && PythonUtil::ThisThreadHasTheGIL()) {
          ReleaseGIL lock;  // 释放GIL
          delete ptr;       // 删除对象
        } else
#endif // LIBCARLA_WITH_PYTHON_SUPPORT
        delete ptr;  // 如果没有Python支持，直接删除对象
      }
    };
  };

} // namespace carla
