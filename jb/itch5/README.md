ITCH-5.0 Messages                    {#itch50page}
=================

Helper classes to parse ITCH-5.0 messages, as documented in the ITCH-5.0
protocol specification (hereafter 'the spec'):

http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf

Each message type defined in the spec is represented by a class.  The
directory also contains some common types, and classes that refactor
the headers common to all messages in the protocol.

In general a message class meets the following constraints:

@code
class foo_message {
public:
  /**
   * The message type identifier.
   *
   * ITCH-5.0 uses a single byte on the wire to identify the type of
   * message.  The identifier is an ASCII character, though in most
   * platforms a character literal (for example 'S') will evaluate to
   * the corresponding ASCII code (83 in the case of 'S'), this is not
   * guaranteed.  We use UTF-16 literals (in this example u'S')
   * because these are available in C++11 and match ASCII for the
   * characters used by the protocol (in C++14 we could use u8'S', but
   * we are not using C++14 features).
   */
  static int const message_type = ...;
};

template<bool V>
class decoder<V,foo_message> {
  /**
   * Decode, and if V==true validate, an ITCH-5.0 buffer into the message type.
   *
   * Extract the field values from the buffer, and validate the fields
   * against the protocol.  The protocol makes many guarantees with
   * respect to field values, for example, prices are only in the
   * [0,200000] range, symbols and the locate codes are constant
   * through the life of a session.  This version of the decode
   * function validates the fields, this is useful during testing to
   * make sure our assumptions about the protocol are correct.
   * However, in a production environment one might prefer to trust
   * the protocol source and use unsafe_decode()
   */
  static foo_message r(std::size_t size, char const* buffer, std::size_t off);
};

@endcode
