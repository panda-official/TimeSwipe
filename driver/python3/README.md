## Build soure distribution

```
python3 setup.py sdist
```

## Upload source distribution

```
python3 -m twine upload --repository-url https://test.pypi.org/legacy/ dist/*
```

## Install on target

```
sudo pip3 install --upgrade pip
sudo pip3 install --upgrade cmake_setuptools
sudo pip3 install -i https://test.pypi.org/simple/ --upgrade timeswipe1
```

## Documentation:

```
pydoc3 timeswipe
```

## Test:

```
sudo python3 test.py
```
