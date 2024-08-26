#pragma once

#include"Constants.h"

NAMESPACE_START

    template<typename Type>
    class TRefCountPtr
    {
        using PtrType = Type;

    public:

        TRefCountPtr() {}

        TRefCountPtr(PtrType* ptr) {
            this->ptr = ptr;
            addRef();
        }

        TRefCountPtr(const TRefCountPtr & otherCounter) {
            
            this->ptr = otherCounter.getPtr();
            otherCounter.addRef();
            this->mRefNum = otherCounter.getRefNum();
        }

        TRefCountPtr(TRefCountPtr&& otherCounter) {

            this->ptr = otherCounter.getPtr();
            this->mRefNum = otherCounter.getRefNum();
            otherCounter.reset();
        }

        TRefCountPtr& operator=(TRefCountPtr& otherCounter) {
        
            if (this->ptr != otherCounter.getPtr()){
        
                this->ptr = otherCounter.getPtr();
                this->mRefNum = otherCounter.getRefNum();
                otherCounter.reset();
            }
            
            return *this;
        }

        inline PtrType* getPtr() const{
            return ptr;
        }

        inline void reset() {
            this->ptr = nullptr;
            this->mRefNum = 0;
        }

        void release(){

            _InterlockedDecrement((long*)(&mRefNum));
            //_MT_INCR(mRefNum);
            if (mRefNum <= 0 && ptr) {
                delete ptr;
                ptr = nullptr;
            }
        }

        virtual ~TRefCountPtr() {
            release();
        };


        unsigned int addRef() const {
            _InterlockedIncrement((long*)(&mRefNum));
            //_MT_DECR(mRefNum);
            return mRefNum;
        }

        inline unsigned int getRefNum() const {
            return mRefNum;
        }

    protected:
        //mutable volatile unsigned int mRefNum = 1;
        mutable volatile unsigned int mRefNum = 0;
        PtrType* ptr = nullptr;
    };


NAMESPACE_END
