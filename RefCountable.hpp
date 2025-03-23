#include <atomic>
#include <stdexcept>
#include <cassert>
#include <utility>

template <typename T>
class RefCounted;

template <typename T>
class RefCountableBase
{
   friend class RefCounted<T>;

public:
   ~RefCountableBase()
   {
      if (counter.load(std::memory_order_relaxed) != 0)
      {
         assert(false && "RefCountable destroyed while back references exist!");

         std::terminate();
      }
   }

   RefCountableBase &operator=(const RefCountableBase &) = delete;
   RefCountableBase &operator=(RefCountableBase &&) = delete;

protected:
   RefCountableBase(T &value) : value{value}, counter{0} {}
   RefCountableBase(const RefCountableBase &rhs) : value{rhs.value}, counter{0} {}
   RefCountableBase(RefCountableBase &&rhs) : value{std::move(rhs.value)}, counter{0} {}

private:
   T &value;
   mutable std::atomic<size_t> counter;
};

template <typename T>
class RefCountable final
{
   friend class RefCounted<T>;

public:
   template <typename... Args>
   RefCountable(Args &&...args) : value{std::forward<Args>(args)...}, counter{0}
   {
   }

   ~RefCountable()
   {
      if (counter.load(std::memory_order_relaxed) != 0)
      {
         assert(false && "RefCountable destroyed while back references exist!");

         std::terminate();
      }
   }

   RefCountable(const RefCountable &rhs) : value{rhs.value}, counter{0}
   {
   }

   T &get() { return value; }
   const T &get() const { return value; }

   RefCountable &operator=(const RefCountable &rhs)
   {
      value = rhs.value();
      return *this;
   }

   RefCountable &operator=(RefCountable &&rhs)
   {
      value = std::move(rhs.value);
      return *this;
   }

   RefCountable &operator=(const T &rhs)
   {
      value = rhs;
      return *this;
   }

   RefCountable &operator=(T &&rhs)
   {
      value = std::move(rhs);
      return *this;
   }

private:
   T value;
   mutable std::atomic<size_t> counter;
};

template <typename T>
class RefCounted
{
public:
   template <typename U>
   RefCounted(RefCountable<U> &ref) : value{ref.value}, counter{ref.counter}
   {
      counter.get().fetch_add(1, std::memory_order_relaxed);
   }

   template <typename U>
   RefCounted(const RefCountable<U> &ref) : value{std::as_const(ref.value)}, counter{ref.counter}
   {
      counter.get().fetch_add(1, std::memory_order_relaxed);
   }

   template <typename U>
   RefCounted(RefCountableBase<U> &ref) : value{ref.value}, counter{ref.counter}
   {
      counter.get().fetch_add(1, std::memory_order_relaxed);
   }

   template <typename U>
   RefCounted(const RefCountableBase<U> &ref) : value{std::as_const(ref.value)}, counter{ref.counter}
   {
      counter.get().fetch_add(1, std::memory_order_relaxed);
   }

   template <typename U>
   RefCounted(RefCounted<U> &rhs) : value{rhs.value}, counter{rhs.counter}
   {
      counter.get().fetch_add(1, std::memory_order_relaxed);
   }

   template <typename U>
   RefCounted(const RefCounted<U> &rhs) : value{std::as_const(rhs.value)}, counter{rhs.counter}
   {
      counter.get().fetch_add(1, std::memory_order_relaxed);
   }

   template <typename U>
   RefCounted(RefCounted<U> &&rhs) : value{rhs.value}, counter{rhs.counter}
   {
      counter.get().fetch_add(1, std::memory_order_relaxed);
   }

   template <typename U>
   RefCounted &operator=(const RefCounted<U> &rhs)
   {
      if (this == &rhs)
         return *this;

      counter.get().fetch_sub(1, std::memory_order_relaxed);

      value = rhs.value;
      counter = rhs.counter;

      counter.get().fetch_add(1, std::memory_order_relaxed);

      return *this;
   }

   ~RefCounted()
   {
      counter.get().fetch_sub(1, std::memory_order_relaxed);
   }

   T &get()
   {
      return value.get();
   }

   const T &get() const
   {
      return value.get();
   }

private:
   std::reference_wrapper<T> value;
   std::reference_wrapper<std::atomic<size_t>> counter;
};
