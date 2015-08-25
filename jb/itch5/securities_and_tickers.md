Securities, Symbols, Tickers, oh my!             {#securities-vs-symbols}
====================================

The ITCH-5.0 protocol suffers from a common confusion in the
securities industry.  It calls their ticker field 'Stock'.  While it
is true that most securities traded on Nasdaq are common stock
securities, many are not: Nasdaq also trades Exchange Traded Funds,
Exchange Traded Notes, and even Warrants.

And the name of a security, i.e., the string used to identify the
security in the many electronic protocols used between exchanges and
participants, is not the same as the security.  A more appropriate
name would have been 'Security Identifier', or 'Ticker', or 'Symbol'.

Software engineers in the industry should be warned to not confuse a
thing (a security: a contract granting specific rights to its owner),
vs. the name of the thing (a ticker: a short string used to identify a
security).

Also, software engineers in the field should be aware that the same
security may be identified by different strings in different contexts,
e.g. many exchanges used different tickers for the same security, yes,
even in the US markets.  That sometimes the same tickers refers to
different securities as time goes by, for example, after some
corporate actions.  That in non-US markets it is common to use fairly
long strings (ISINs) or just numbers (in Japan) to identify
securities.  And that the string used to identify securities in a
market feed may not be the same string used to identify them when
placing orders, clearing, etc.


References
----------

http://en.wikipedia.org/wiki/Haddocks%27_Eyes
