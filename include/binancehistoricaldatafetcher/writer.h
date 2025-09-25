//
// Created by jtwears on 9/16/25.
//

# pragma once


namespace writer {
    class IWriter {
    public:
        virtual ~IWriter() = default;
        virtual void write();
        virtual void close();
    };

}
