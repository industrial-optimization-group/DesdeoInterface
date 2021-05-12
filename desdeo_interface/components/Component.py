from pyfirmata import Board, Pin

class Component:
    """
    A base class for components
    Args:
        board (pyfirmata.pyfirmata.Board): The board (Arduino) the component is connected
        pin (int): The pin the component is connected to
    Raises:
        Exception: pin doesn't exist on the arduino uno
    """

    pin: Pin

    def __init__(self, board: Board, pin: int, is_digital: bool):
        if is_digital:
            if pin < 2 or pin > 13:
                raise Exception("Pin is invalid, should be in the range on 2-13")
            self.pin = board.get_pin(f'd:{pin}:i')
        else: #Is analog
            if pin < 0 or pin > 5:
                raise Exception("Pin is invalid, should be in the range on 0-5")
            self.pin = board.get_pin(f'a:{pin}:i')
    
    def get_pin_value(self):
        """
        Read the value of the pin assigned to the component
        Returns:
            float | None: value of the pin which is between 0.0 - 1.0 or None if pin is unavailable
        """
        return self.pin.read()
