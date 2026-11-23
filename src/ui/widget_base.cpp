#include <ui/widget_base.hpp>

namespace axiom {

container_widget* widget::container() {
    return dynamic_cast<container_widget*>(this);
}

}