So, memory management in Radiance is a clusterfuck. Here's why:

1. Radiance uses QML. That's how it can have such a snazzy UI and a Javascript scripting engine, which are both really nice things.
2. Radiance uses multiple threads for rendering. This way, the Javacsript UI can't block output, and a slow renderer won't make the UI unresponsive. These are also both really nice things.

Because multiple threads may be using the same `VideoNode`, they can't use Qt's parent-child memory management paradigm. There is not a safe parent to delete them. They must use a reference counting smart pointer type memory management scheme. We use `QSharedPointer` for this. This way, the UI as well as each render thread can claim a stake in a `VideoNode`'s continued survival.

QML hates shared pointers. QML requires using a parent-child memory management structure. If you don't, it will sometimes give your objects to the Javascript garbage collector (e.g. if you call a function such as `getVideoNode()`) and sometimes will assume that C++ will delete the object (e.g. if you access a property such as `videoNode`.) The only way to get things into QML is with a raw pointer to a `QObject`.

## Radiance's Solution

The way Radiance handles this basically boils down to two things:

1. Always use smart pointers
2. When outside of the rendere (e.g. high-level C++ or QML), wrap the smart pointer in a QObject, and manage that particular reference using parent/child (or the JS GC.)

The wrapper class is called `QmlSharedPointer` and extends `QSharedPointer` as well as `QObject`. It fakes having all of the signals, slots, and properties of the class that it wraps. It's quite a piece of work.

Radiance defines typedefs for the various types of wrapped `VideoNode`. A `QmlSharedPointer<VideoNode>` is called a `VideoNodeSP`.

Unfortunately, since the only way to get things into QML is with raw pointers, we end up using a lot of things like `VideoNodeSP *v`.
That's a *raw* pointer to a *smart* pointer.
When you `delete v`, it dereferences the smart pointer and will delete the `VideoNode` only if nobody else is using it.

This is exactly the behavior we want, but it can be hard to wrap your head around.

This sounds simple, but in effect it creates a very complex situation for the programmer that is hard to get right.

## FAQ

### Do I use `QSharedPointer<VideoNode>` or `QmlSharedPointer<VideoNode>` (`VideoNodeSP`)?

The general rule here is to use a `QSharedPointer<VideoNode>` whenever you are in C++,
working "low-level" or on the "back-end." For the most part, this means the renderer.

Use a `VideoNodeSP *` whenever you need to interface with QML or are working at a higher level.
Use it when the caller wants to live a life of blissful ignorance about the threaded rendering madhouse behind the scenes.
Let the caller pretend that it just got a `QObject` and has to manage it in the normal Qt way.
An example of high-level C++ is `View.cpp` whose interface should only use `VideoNodeSP *`.

You're probably doing it wrong if you have:

- A `VideoNodeSP` that is not a pointer. You normally can't copy `QObject`, so they are always passed as pointers.
  You *can* copy `VideoNodeSP` but you never should.
  However, you can claim another reference to it by
  casting it to a `QSharedPointer<VideoNode>` for use in the backend.

- An object whose API features both `VideoNodeSP *` and `QSharedPointer<VideoNode>`.
  You should decide whether the object you're working on is high-level or low-level,
  and pick `VideoNodeSP *` or `QSharedPointer<VideoNode>` accordingly.

- A raw `VideoNode *`. This should either be `VideoNodeSP *` or a `QSharedPointer<VideoNode>`.

- An object whose API features `VideoNodeSP *` setters / getters but whose backing member variable is a `QSharedPointer<VideoNode>`.
  If you are doing this, you are probably accidentally creating a new `VideoNodeSP *` whenever the accessor is called,
  which is not how QObject-accessors should work. You are probably leaking memory
  and causing the backing `VideoNode` to stick around forever.

- An object whose API features `QSharedPointer<VideoNode>` setters / getters but whose backing member variable is a `VideoNodeSP *`.
  There's actually nothing technically wrong with this, it's just poor style.

### How does upcasting and downcasting work?

- TBD

### If I have a `VideoNodeSP *v`, what is the difference between `v == nullptr` and `v->isNull()`?

This one's easy. You should never encounter a situation where `v->isNull()`.
`VideoNodeSP` *can* wrap `nullptr` but it never should.

`v == nullptr` is the way to indicate a lack of `VideoNode` or an erroneous return code.

This is how things normally work with `QObject` and QML, and so we try to mimic that as best as we can.

When using `QSharedPointer<VideoNode>` it is okay for `.isNull()` to be `true`.
Just be careful when turning it into a `VideoNodeSP *`.
It should be turned into a `nullptr`, not a pointer to a `VideoNodeSP(nullptr)`.

### Who `delete`s objects passed into QML land?

TBD, read [this](https://wiki.qt.io/Shared_Pointers_and_QML_Ownership) for now to get some idea.

### I hate the pattern of `(*videoNode)->method` when working on `VideoNodeSP *`

Too bad, that's how we have to do it. Also be sure to check `videoNode != nullptr` before doing that.
