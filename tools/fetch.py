"""Downloads for build.py and extract_assets.py that work with any Python on the PATH.

Some programs bring a Python of their own that can't verify HTTPS certificates: Inkscape's, for one
(issue #52), whose OpenSSL has no certificate store. A python.org install on macOS is the same until
its "Install Certificates" step has run. urllib then fails with CERTIFICATE_VERIFY_FAILED before
anything is downloaded.

So: urllib first; if the certificate can't be verified, the system's own curl, which uses the
system's certificate store (on Windows 10 and later, C:\\Windows\\System32\\curl.exe and Windows'
store; on macOS, the keychain). Certificates are always verified, one way or the other. If neither
works, the error says which Python it was and what to do.

To try the fallback on a machine whose Python is fine, set A3MX_TEST_NO_CERTS=1: urllib is then
treated as having failed the certificate check (and build.cmd passes over your Python too).

Standard library only.
"""
import os, shutil, ssl, subprocess, sys, tempfile, urllib.error, urllib.request

WINDOWS = os.name == 'nt'


class FetchError(Exception):
    pass


def cert_failure(ex):
    """True if EX is urllib failing to verify the server's certificate."""
    if isinstance(ex, ssl.SSLCertVerificationError): return True
    return isinstance(ex, urllib.error.URLError) and isinstance(getattr(ex, 'reason', None), ssl.SSLCertVerificationError)


def system_curl():
    """The system's curl: Windows' own in System32 first (a curl another program put on the PATH may
    have no certificates either), else whichever is on the PATH."""
    if WINDOWS:
        own = os.path.join(os.environ.get('SystemRoot', r'C:\Windows'), 'System32', 'curl.exe')
        if os.path.isfile(own): return own
    return shutil.which('curl')


def advice():
    return ('This Python (%s) cannot verify HTTPS certificates, and the system\'s curl could not download '
            'it either. %s' % (sys.executable,
            'Install Python from https://www.python.org (tick "Add python.exe to PATH"), or run build.cmd, '
            'which uses its own Python when yours cannot do this.' if WINDOWS else
            'On macOS, run "Install Certificates.command" in your Python folder; elsewhere, install your '
            'system\'s CA certificates (e.g. the ca-certificates package).'))


def urlopen_to(url, f, timeout):
    if os.environ.get('A3MX_TEST_NO_CERTS'):                # testing the fallback (see above)
        raise ssl.SSLCertVerificationError(1, 'certificate verify failed (A3MX_TEST_NO_CERTS is set)')
    with urllib.request.urlopen(url, timeout=timeout) as r: shutil.copyfileobj(r, f)


def fetch_to(url, path, timeout=120):
    """Download URL into the file PATH (replaced only once the download is complete)."""
    tmp = path + '.part'
    try:
        try:
            with open(tmp, 'wb') as f: urlopen_to(url, f, timeout)
        except Exception as ex:
            if not cert_failure(ex): raise
            curl = system_curl()
            print('  this Python cannot verify HTTPS certificates (%s): downloading with %s'
                  % (sys.executable, curl or 'curl (none found)')); sys.stdout.flush()
            if not curl: raise FetchError(advice())
            r = subprocess.run([curl, '-fsSL', '--retry', '3', '--max-time', str(max(timeout, 600)), '-o', tmp, url])
            if r.returncode != 0: raise FetchError('curl exit code %d. %s' % (r.returncode, advice()))
        os.replace(tmp, path)
    finally:
        if os.path.exists(tmp): os.remove(tmp)


def fetch(url, timeout=120):
    """Download URL and return its bytes."""
    fd, path = tempfile.mkstemp(suffix='.download'); os.close(fd)
    try:
        fetch_to(url, path, timeout)
        with open(path, 'rb') as f: return f.read()
    finally:
        os.remove(path)
