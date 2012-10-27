#include <iterator>
#include <memory>
using namespace std;

template < class Key, class T >
class ordered_forward_map_element {
friend class ordered_forward_map;
friend class ordered_forward_map_iterator;
protected:
    typedef ordered_forward_map_element<Key, T> class_t;
    const Key key;
    T value;
    class_t* nextElement;
    
    ordered_forward_map_element(T value = T(), class_t* nextElement = NULL):
                                 value(value), nextElement(nextElement) { }
    ordered_forward_map_element(const class_t& element):
            value(element.value), nextElement(element.nextElement) { }
};

template < class Key, class T >
class ordered_forward_map_iterator : public iterator<forward_iterator_tag, T> {
friend class ordered_forward_map;
protected:
    typedef ordered_forward_map_iterator<Key, T> class_t;
    typedef ordered_forward_map_element<Key, T> element_t;
    element_t* element;
    
public:
    ordered_forward_map_iterator(element_t* element = NULL):
                                 element(element) { }
    ordered_forward_map_iterator(const class_t& iterator):
                                element(iterator.element) { }
    
    class_t& operator = (class_t& b) {
        element = b.element;
        return this;
    }
    bool operator == (class_t& b) {
        return (this->element == b.element);
    }
    bool operator != (class_t& b) {
        return !(*this == b);
    }
    T& operator * () {
        return element->value;
    }
    class_t& operator ++ () {     //++class_t
        element = element->nextElement;
        return *this;
    }
    class_t operator ++ (int) {  //class_t++
        class_t tmp(*this);
        element = element->nextElement;
        return tmp;
    }
};

template < class Key, class T, class Compare = less<T> >
class ordered_forward_map {
//---------------Type Defs-------------------
public:
    typedef T value_type;
    typedef Key key_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef ordered_forward_map_iterator<Key, T> iterator;
    typedef ordered_forward_map_iterator<Key, const T> const_iterator;
    typedef size_t size_type;
    typedef ordered_forward_map<Key, T, Compare> class_t;
    typedef ordered_forward_map_element<Key, T> element_t;
    
//---------------Variables-------------------
protected:
    element_t* firstElement;

//-------------Constructors------------------
public:
    //empty (default)
    explicit ordered_forward_map (): firstElement(NULL) { }
    //copy
    ordered_forward_map (const class_t& fwdmap): firstElement(NULL) {
        *this = fwdmap;
    }

//--------------Destructor-------------------
protected:
    void destruct_internals() {
        if (firstElement != NULL) {
            element_t* curElement = firstElement;
            while (curElement->nextElement != NULL) {
                element_t* tmp = curElement->nextElement;
                delete curElement;
                curElement = tmp;
            }
            delete curElement;
            curElement = NULL;
        }
    }
public:
    ~ordered_forward_map() {
        destruct_internals();
    }
    
//---------------Operators-------------------
    //copy
    class_t& operator = ( const class_t& fwdmap ) {
        destruct_internals();
        if (fwdmap.firstElement != NULL) {
            firstElement = new element_t(&(fwdmap.firstElement));
            element_t* curThisElement = firstElement;
            element_t* curElement = fwdmap.firstElement;
            while (curElement.nextElement != NULL) {
                curElement = curElement->nextElement;
                curThisElement->nextElement = new element_t(&curElement);
                curThisElement = curThisElement->nextElement;
            }
        }
        return *this;
    }

//---------------Iterators-------------------
    iterator begin() noexcept {
        return iterator(firstElement);
    }
    const_iterator cbegin() const noexcept {
        return const_iterator(firstElement);
    }

//----------------Capacity-------------------
    bool empty() const noexcept {
        return firstElement == NULL;
    }

//------------Element access-----------------
    reference front() {
        return firstElement->value;
    }

//---------------Modifiers-------------------
    void pop_front () {
        element_t* tmp = firstElement;
        if (tmp != NULL) {
            firstElement = firstElement->nextElement;
            delete tmp;
        }
    }
    
    iterator insert (const key_type key, const value_type& val) {
        element_t* lastElement = NULL;
        element_t* curElement = firstElement;
        while (true) {
            if ((curElement == NULL) || Compare(key, curElement->key)) {
                element_t* newElement = element_t(key, val, curElement);
                if (lastElement == NULL)
                    firstElement = newElement;
                else
                    lastElement->nextElement = newElement;
                return iterator(newElement);
            }
            lastElement = curElement;
            curElement = curElement->nextElement;
        }
    }

    void erase_after ( iterator position ) {
        element_t* beforeElement = position->element;
        element_t* oldElement = beforeElement->nextElement;
        
        beforeElement->nextElement = oldElement->nextElement;
        delete oldElement;
    }
    size_type erase ( const key_type& key ) {
        size_type num = 0;
        element_t* lastElement = NULL;
        element_t* curElement = firstElement;
        while ((curElement != NULL) && Compare(curElement->key, key)) {
            if (Compare(key, curElement->key)) {
                element_t* oldElement = curElement;
                curElement = curElement->nextElement;
                if (lastElement == NULL) {
                    pop_front();
                } else {
                    lastElement->nextElement = curElement;
                    delete oldElement;
                }
                num++;
                continue;
            }
            lastElement = curElement;
            curElement = curElement->nextElement;
        }
        return num;
    }
    void erase_after( iterator first, iterator last ) {
        if (first == last) return;
        element_t* beforeElement = first->element;
        if (beforeElement == NULL) return;
        element_t* curElement = beforeElement->nextElement;
        element_t* lastElement = last->element;
        while ((curElement != NULL) && (curElement != lastElement)) {
            element_t* oldElement = curElement;
            curElement = curElement->nextElement;
            delete oldElement;
        }
        beforeElement->nextElement = curElement;
    }

    void swap(class_t& fwdlst) {
        element_t* tmp = fwdlst->firstElement;
        fwdlst->firstElement = firstElement;
        firstElement = tmp;
    }
    
    void clear() noexcept {
        destruct_internals();
    }
};
