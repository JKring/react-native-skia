#pragma once

#include <JsiHostObject.h>
#include <RNSkPlatformContext.h>
#include <RNSkMeasureTime.h>
#include <RNSkReadonlyValue.h>
#include <RNSkAnimation.h>
#include <jsi/jsi.h>

namespace RNSkia
{
using namespace facebook;
/**
 Implements a Value that can be both read and written to. It inherits from the ReadonlyValue with
 functionailty for subscribing to changes.
 */
class RNSkValue : public RNSkReadonlyValue
{
public:
  RNSkValue(std::shared_ptr<RNSkPlatformContext> platformContext,
            jsi::Runtime& runtime, const jsi::Value *arguments, size_t count)
      : RNSkReadonlyValue(platformContext) {
        if(count == 1) {
          update(runtime, arguments[0]);
        }
      }
  
  ~RNSkValue() {
    unsubscribe();
  }
  
  JSI_PROPERTY_SET(value) {
    // When someone else is setting the value we need to stop any ongoing
    // animations
    unsubscribe();
    update(runtime, value);
  }
  
  JSI_PROPERTY_SET(animation) {
    // Cancel existing animation
    unsubscribe();
    
    // Verify input
    if(value.isObject() && value.asObject(runtime).isHostObject(runtime)) {
      auto animation = value.asObject(runtime).getHostObject<RNSkAnimation>(runtime);
      if(animation != nullptr) {
        // Now we have a value animation - let us connect and start
        subscribe(animation);
      }
    } else if(value.isUndefined() || value.isNull()) {
      // Do nothing - we've already unsubscribed
    } else {
      jsi::detail::throwJSError(runtime, "Animation expected.");
    }
  }
  
  JSI_PROPERTY_GET(animation) {
    if(_animation != nullptr) {
      return jsi::Object::createFromHostObject(runtime, _animation);
    }
    return jsi::Value::undefined();
  }
  
  JSI_EXPORT_PROPERTY_SETTERS(JSI_EXPORT_PROP_SET(RNSkValue, value),
                              JSI_EXPORT_PROP_SET(RNSkValue, animation))
  
  JSI_EXPORT_PROPERTY_GETTERS(JSI_EXPORT_PROP_GET(RNSkReadonlyValue, __typename__),
                              JSI_EXPORT_PROP_GET(RNSkValue, value),
                              JSI_EXPORT_PROP_GET(RNSkValue, animation))
  
  JSI_EXPORT_FUNCTIONS(
    JSI_EXPORT_FUNC(RNSkValue, addListener),
  )

private:
  void subscribe(std::shared_ptr<RNSkAnimation> animation) {
    unsubscribe();
    if(animation != nullptr) {
      _animation = animation;
      auto dispatch = std::bind(&RNSkValue::animationDidUpdate, this, std::placeholders::_1);
      _unsubscribe = std::make_shared<std::function<void()>>(_animation->addListener(dispatch));
      // Start the animation
      _animation->startClock();
    }
  }
  
  void animationDidUpdate(jsi::Runtime& runtime) {
    if(_animation != nullptr) {
      // Update ourselves from the current animation value
      update(runtime, _animation->get_value(runtime));
    }
  }
  
  void unsubscribe() {
    if(_unsubscribe != nullptr) {
      (*_unsubscribe)();
      _unsubscribe = nullptr;
    }
    
    if(_animation != nullptr) {
      _animation->stopClock();
      _animation = nullptr;
    }
  }
  
  std::shared_ptr<RNSkAnimation> _animation;
  std::shared_ptr<std::function<void()>> _unsubscribe;
};

}
