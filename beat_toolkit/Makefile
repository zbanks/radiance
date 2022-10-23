.PHONY: setup check run

setup:
	poetry install --all-extras

check:
	poetry run isort .
	poetry run black .
	poetry run flake8 .
	poetry run mypy .

run:
	cd .. && cargo build --profile release --bin beat_tracking_test
	poetry run pytest
