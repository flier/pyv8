import sys
import logging
from urllib2 import Request, urlopen, HTTPError
from urlparse import urlparse

import PyV8, w3c

import BeautifulSoup

class Navigator(PyV8.JSClass):
    @property
    def appCodeName(self):
        """the code name of the browser"""
        raise NotImplementedError()

    @property
    def appName(self):
        """the name of the browser"""
        raise NotImplementedError()

    @property
    def appVersion(self):
        """the version information of the browser"""
        raise NotImplementedError()

    @property
    def cookieEnabled(self):
        """whether cookies are enabled in the browser"""
        raise NotImplementedError()

    @property
    def platform(self):
        """which platform the browser is compiled"""
        raise NotImplementedError()

    @property
    def userAgent(self):
        """the user-agent header sent by the browser to the server"""
        raise NotImplementedError()

    def javaEnabled(self):
        """whether or not the browser has Java enabled"""
        raise NotImplementedError()

    def taintEnabled(self):
        """whether or not the browser has data tainting enabled"""
        raise NotImplementedError()

    def fetch(self, url):
        request = Request(url)
        request.add_header('User-Agent', self.userAgent)

        response = urlopen(request)

        if response.code != 200:
            raise HTTPError(url, response.code, "fail to fetch HTML", response.info(), 0)

        return response.read()

class InternetExplorer(Navigator):
    @property
    def appCodeName(self):
        return "Mozilla"

    @property
    def appName(self):
        return "Microsoft Internet Explorer"

    @property
    def appVersion(self):
        return "4.0 (compatible; MSIE 6.0; Windows NT 5.1)"

    @property
    def cookieEnabled(self):
        """whether cookies are enabled in the browser"""
        raise True

    @property
    def platform(self):
        return "Win32"

    @property
    def userAgent(self):
        return "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)"

    def javaEnabled(self):
        return False

    def taintEnabled(self):
        return False

    @property
    def userLanguage(self):
        import locale

        return locale.getdefaultlocale()[0]

class HtmlLocation(PyV8.JSClass):
    def __init__(self, win):
        self.win = win

    @property
    def parts(self):
        return urlparse.urlparse(self.win.url)

    @property
    def href(self):
        return self.win.url

    @href.setter
    def href(self, url):
        self.win.open(url)

    @property
    def protocol(self):
        return self.parts.scheme

    @property
    def host(self):
        return self.parts.netloc

    @property
    def hostname(self):
        return self.parts.hostname

    @property
    def port(self):
        return self.parts.port

    @property
    def pathname(self):
        return self.parts.path

    @property
    def search(self):
        return self.parts.query

    @property
    def hash(self):
        return self.parts.fragment

    def assign(self, url):
        """Loads a new HTML document."""
        self.win.open(url)

    def reload(self):
        """Reloads the current page."""
        self.win.open(self.win.url)

    def replace(self, url):
        """Replaces the current document by loading another document at the specified URL."""
        self.win.open(url)

class Screen(PyV8.JSClass):
    def __init__(self, width, height, depth=32):
        self._width = width
        self._height = height
        self._depth = depth

    @property
    def availWidth(self):
        return self._width

    @property
    def availHeight(self):
        return self._height

    @property
    def width(self):
        return self._width

    @property
    def height(self):
        return self._height

    @property
    def colorDepth(self):
        return self._depth

    @property
    def pixelDepth(self):
        return self._depth

class HtmlWindow(PyV8.JSClass):
    class Timer(object):
        def __init__(self, code, repeat, lang='JavaScript'):
            self.code = code
            self.repeat = repeat
            self.lang = lang

    timers = []

    def __init__(self, navigator, dom_or_doc, name="", target='_blank', parent=None, opener=None, width=800, height=600, left=0, top=0):
        self._navigator = navigator
        self.doc = w3c.getDOMImplementation(dom_or_doc) if isinstance(dom_or_doc, BeautifulSoup) else dom_or_doc
        self.loc = HtmlLocation(self)

        self._target = target
        self._parent = parent
        self._opener = opener
        self._screen = Screen(width, height, 32)
        self._closed = False

        self.name = name
        self.defaultStatus = ""
        self.status = ""
        self._left = left
        self._top = top
        self.innerWidth = width
        self.innerHeight = height
        self.outerWidth = width
        self.outerHeight = height

    @property
    def context(self):
        if not hasattr(self, '_context'):
            self._context = PyV8.JSContext(self)

        return self._context

    @property
    def closed(self):
        """whether a window has been closed or not"""
        return self._closed

    def close(self):
        """Closes the current window"""
        self._closed = True

    @property
    def window(self):
        return self

    @property
    def document(self):
        return self.doc

    @property
    def frames(self):
        """an array of all the frames (including iframes) in the current window"""
        return w3c.HTMLCollection(self.doc, [self.doc.createHTMLElement(self.doc, f) for f in self.doc.find(['frame', 'iframe'])])

    @property
    def length(self):
        """the number of frames (including iframes) in a window"""
        return len(self.doc.find(['frame', 'iframe']))

    @property
    def history(self):
        """the History object for the window"""
        raise NotImplementedError()

    @property
    def location(self):
        """the Location object for the window"""
        return self.loc

    @property
    def navigator(self):
        """the Navigator object for the window"""
        return self._navigator

    @property
    def opener(self):
        """a reference to the window that created the window"""
        return self._opener

    @property
    def pageXOffset(self):
        return 0

    @property
    def pageYOffset(self):
        return 0

    @property
    def parent(self):
        return self._parent

    @property
    def screen(self):
        return self._screen

    @property
    def screenLeft(self):
        return self._left

    @property
    def screenTop(self):
        return self._top

    @property
    def screenX(self):
        return self._left

    @property
    def screenY(self):
        return self._top

    @property
    def self(self):
        return self

    @property
    def top(self):
        return self

    def alert(self, msg):
        """Displays an alert box with a message and an OK button"""
        print "ALERT: ", msg

    def confirm(self, msg):
        """Displays a dialog box with a message and an OK and a Cancel button"""
        ret = raw_input("CONFIRM: %s [Y/n] " % msg)

        return ret in ['', 'y', 'Y', 't', 'T']

    def focus(self):
        """Sets focus to the current window"""
        pass

    def blur(self):
        """Removes focus from the current window"""
        pass

    def moveBy(self, x, y):
        """Moves a window relative to its current position"""
        pass

    def moveTo(self, x, y):
        """Moves a window to the specified position"""
        pass

    def resizeBy(self, w, h):
        """Resizes the window by the specified pixels"""
        pass

    def resizeTo(self, w, h):
        """Resizes the window to the specified width and height"""
        pass

    def scrollBy(self, xnum, ynum):
        """Scrolls the content by the specified number of pixels"""
        pass

    def scrollTo(self, xpos, ypos):
        """Scrolls the content to the specified coordinates"""
        pass

    def setTimeout(self, code, interval, lang="JavaScript"):
        timer = HtmlWindow.Timer(code, False, lang)
        self.timers.append((interval, timer))

        return len(self.timers)-1

    def clearTimeout(self, idx):
        self.timers[idx] = None

    def setInterval(self, code, interval, lang="JavaScript"):
        timer = HtmlWindow.Timer(code, True, lang)
        self.timers.append((interval, timer))

        return len(self.timers)-1

    def clearInterval(self, idx):
        self.timers[idx] = None

    def createPopup(self):
        raise NotImplementedError()

    def open(self, url, name='_blank', specs='', replace=False):
        kwds = {}

        for spec in specs.split(','):
            spec = [s.strip() for s in spec.split('=')]

            if len(spec) == 2:
                if spec[0] in ['width', 'height', 'left', 'top']:
                    kwds[spec[0]] = int(spec[1])

        if name in ['_blank', '_parent', '_self', '_top']:
            kwds['target'] = name
            name = ''
        else:
            kwds['target'] = '_blank'

        html = self._navigator.fetch(url)        
        dom = w3c.parseString(html)

        return HtmlWindow(self._navigator, dom, name, parent=self, opener=self, **kwds)

    def Image(self):
        return self.doc.createElement('img')

import unittest

if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG if "-v" in sys.argv else logging.WARN,
                        format='%(asctime)s %(levelname)s %(message)s')

    unittest.main()