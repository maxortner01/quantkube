// Create a service which is essentially just a string mapping to a lambda or function interface
// Then we can automatically do the flatbuffer deserialization then pass the payload to the correct endpoint
// The user should specify the return type, which should be a flatbuffer object
// When this object is returned from the lambda, we serialize it and send it back to the client